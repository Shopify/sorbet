#include "rbs/RBSRewriter.h"

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "common/typecase.h"
#include "core/errors/rewriter.h"
#include "parser/Builder.h"
#include "parser/helper.h"
#include "parser/parser.h"
#include "parser/parser/include/ruby_parser/builder.hh"
#include "parser/parser/include/ruby_parser/driver.hh"
#include "parser/parser/include/ruby_parser/token.hh"
#include "rbs/MethodTypeTranslator.h"
#include "rbs/RBSParser.h"
#include "rbs/TypeTranslator.h"
#include "rbs/rbs_common.h"
#include <regex>

using namespace std;

namespace sorbet::rbs {

namespace {

unique_ptr<parser::Node> insertCast(unique_ptr<parser::Node> node,
                                    optional<pair<unique_ptr<parser::Node>, InlineComment::Kind>> pair) {
    if (!pair) {
        return node;
    }

    auto type = move(pair->first);
    auto kind = pair->second;

    if (kind == InlineComment::Kind::LET) {
        return parser::MK::TLet(type->loc, move(node), move(type));
    } else if (kind == InlineComment::Kind::CAST) {
        return parser::MK::TCast(type->loc, move(node), move(type));
    } else if (kind == InlineComment::Kind::MUST) {
        return parser::MK::TMust(type->loc, move(node));
    } else {
        Exception::raise("Unknown assertion kind");
    }
}

} // namespace

/**
 * Check if the given range contains a heredoc marker `<<-` or `<<~`
 */
bool RBSRewriter::hasHeredocMarker(const uint32_t fromPos, const uint32_t toPos) {
    string source(ctx.file.data(ctx).source().substr(fromPos, toPos - fromPos));
    regex heredoc_pattern("(\\s+=\\s<<-|~)");
    smatch matches;
    return regex_search(source, matches, heredoc_pattern);
}

/**
 * Check if the given expression is a heredoc
 */
bool RBSRewriter::isHeredoc(core::LocOffsets assignLoc, const unique_ptr<parser::Node> &node) {
    if (node == nullptr) {
        return false;
    }

    auto result = false;
    typecase(
        node.get(),
        [&](parser::String *lit) {
            // For some reason, heredoc strings are parser differently if they contain a single line or more.
            //
            // Single line heredocs do not contain the `<<-` or `<<~` markers inside their location.
            //
            // For example, this heredoc:
            //
            //     <<~MSG
            //       foo
            //     MSG

            // has the `<<-` or `<<~` markers **outside** its location.
            //
            // While this heredoc:
            //
            //     <<~MSG
            //       foo
            //     MSG
            //
            // has the `<<-` or `<<~` markers **inside** its location.

            auto lineStart = core::Loc::pos2Detail(ctx.file.data(ctx), lit->loc.beginLoc).line;
            auto lineEnd = core::Loc::pos2Detail(ctx.file.data(ctx), lit->loc.endLoc).line;

            if (lineEnd - lineStart <= 1) {
                // Single line heredoc, we look for the heredoc marker outside, ie. between the assign `=` sign
                // and the begining of the string.
                if (hasHeredocMarker(assignLoc.endPos(), lit->loc.beginPos())) {
                    result = true;
                }
            } else {
                // Multi-line heredoc, we look for the heredoc marker inside the string itself.
                if (hasHeredocMarker(lit->loc.beginPos(), lit->loc.endPos())) {
                    result = true;
                }
            }
        },
        [&](parser::DString *lit) {
            // For some reason, heredoc strings are parser differently if they contain a single line or more.
            //
            // Single line heredocs do not contain the `<<-` or `<<~` markers inside their location.
            //
            // For example, this heredoc:
            //
            //     <<~MSG
            //       foo
            //     MSG

            // has the `<<-` or `<<~` markers **outside** its location.
            //
            // While this heredoc:
            //
            //     <<~MSG
            //       foo
            //     MSG
            //
            // has the `<<-` or `<<~` markers **inside** its location.

            auto lineStart = core::Loc::pos2Detail(ctx.file.data(ctx), lit->loc.beginLoc).line;
            auto lineEnd = core::Loc::pos2Detail(ctx.file.data(ctx), lit->loc.endLoc).line;

            if (lineEnd - lineStart <= 1) {
                // Single line heredoc, we look for the heredoc marker outside, ie. between the assign `=` sign
                // and the begining of the string.
                if (hasHeredocMarker(assignLoc.endPos(), lit->loc.beginPos())) {
                    result = true;
                }
            } else {
                // Multi-line heredoc, we look for the heredoc marker inside the string itself.
                if (hasHeredocMarker(lit->loc.beginPos(), lit->loc.endPos())) {
                    result = true;
                }
            }
        },
        [&](parser::Send *send) {
            result = isHeredoc(assignLoc, send->receiver) ||
                     absl::c_any_of(send->args,
                                    [&](const unique_ptr<parser::Node> &arg) { return isHeredoc(assignLoc, arg); });
        },
        [&](parser::Node *expr) { result = false; });

    return result;
}

optional<rbs::InlineComment> RBSRewriter::commentForPos(uint32_t fromPos) {
    auto source = ctx.file.data(ctx).source();

    // Get the position of the end of the line from the startingLoc
    auto endPos = source.find('\n', fromPos);
    if (endPos == string::npos) {
        // If we don't find a newline, we just use the rest of the file
        endPos = source.size();
    }

    if (fromPos == endPos) {
        // If the start and end of the comment are the same, we don't have a comment
        return nullopt;
    }

    // Find the position of the `#:` between the fromPos and the end of the line
    auto commentStart = source.substr(0, endPos).find("#:", fromPos);
    if (commentStart == string::npos) {
        return nullopt;
    }

    // Adjust the location to be the correct position depending on the number of spaces after the `#:`
    auto contentStart = commentStart + 2;
    char c = source[contentStart];
    while (c == ' ' && contentStart < endPos) {
        contentStart++;
        c = source[contentStart];
    }

    auto content = source.substr(contentStart, endPos - contentStart);
    auto kind = InlineComment::Kind::LET;

    if (absl::StartsWith(content, "as ")) {
        // We found a `as` keyword, this is a `cast` comment
        kind = InlineComment::Kind::CAST;
        contentStart += 3;
        content = content.substr(3);

        const regex not_nil_pattern("^\\s*!nil\\s*(#.*)?$");
        if (regex_match(content.begin(), content.end(), not_nil_pattern)) {
            // We found a `as !nil`, so a must
            kind = InlineComment::Kind::MUST;
        }
    }

    return InlineComment{
        rbs::Comment{core::LocOffsets{(uint32_t)commentStart, static_cast<uint32_t>(endPos)},
                     core::LocOffsets{(uint32_t)contentStart, static_cast<uint32_t>(endPos)}, content},
        kind,
    };
}

optional<rbs::InlineComment> RBSRewriter::commentForNode(unique_ptr<parser::Node> &node, core::LocOffsets fromLoc) {
    // We want to find the comment right after the end of the assign
    auto fromPos = node->loc.endPos();

    // On heredocs, adding the comment at the end of the assign won't work because this is invalid Ruby syntax:
    // ```
    // <<~MSG
    //   foo
    // MSG #: String
    // ```
    // We add a special case for heredocs to allow adding the comment at the end of the assign:
    // ```
    // <<~MSG #: String
    //   foo
    // MSG
    // ```
    if (isHeredoc(fromLoc, node)) {
        fromPos = fromLoc.beginPos();
    }

    return commentForPos(fromPos);
}

/**
 * Get the RBS type from the given assign
 */
optional<pair<unique_ptr<parser::Node>, InlineComment::Kind>>
RBSRewriter::assertionForNode(unique_ptr<parser::Node> &node, core::LocOffsets fromLoc) {
    auto inlineComment = commentForNode(node, fromLoc);

    if (!inlineComment) {
        return nullopt;
    }

    if (inlineComment->kind == InlineComment::Kind::MUST) {
        return pair<unique_ptr<parser::Node>, InlineComment::Kind>{
            make_unique<parser::Nil>(inlineComment->comment.loc),
            inlineComment->kind,
        };
    }

    auto result = rbs::RBSParser::parseType(ctx, inlineComment->comment);
    if (result.second) {
        if (auto e = ctx.beginError(result.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type ({})", result.second->message);
        }
        return nullopt;
    }

    auto rbsType = move(result.first.value());
    auto typeParams = lastTypeParams();
    return pair{rbs::TypeTranslator::toParserNode(ctx, typeParams, rbsType.node.get(), inlineComment->comment.loc),
                inlineComment->kind};
}

/**
 * Parse the type parameters from the previous statement
 *
 * Given a case like this one:
 *
 *     #: [X] (X) -> void
 *     def foo(x)
 *       y = nil #: X?
 *     end
 *
 * We need to be aware of the type parameter `X` so we can use it to resolve the type of `y`.
 */
vector<pair<core::LocOffsets, core::NameRef>> RBSRewriter::lastTypeParams() {
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>();

    // Do we have a previous signature?
    if (!lastSignature) {
        return typeParams;
    }

    auto block = parser::cast_node<parser::Block>(lastSignature);
    ENFORCE(block != nullptr);

    // Does the sig contain a `type_parameters()` invocation?
    auto send = parser::cast_node<parser::Send>(block->body.get());
    while (send && send->method != core::Names::typeParameters()) {
        send = parser::cast_node<parser::Send>(send->receiver.get());
    }

    if (send == nullptr) {
        return typeParams;
    }

    // Collect the type parameters
    for (auto &arg : send->args) {
        auto sym = parser::cast_node<parser::Symbol>(arg.get());
        if (sym == nullptr) {
            continue;
        }

        typeParams.emplace_back(arg->loc, sym->val);
    }

    return typeParams;
}

void RBSRewriter::maybeSaveSignature(parser::Block *block) {
    if (block->body == nullptr) {
        return;
    }

    auto send = parser::cast_node<parser::Send>(block->send.get());
    if (send == nullptr) {
        return;
    }

    if (send->method != core::Names::sig()) {
        return;
    }

    lastSignature = block;
}

// unique_ptr<parser::Node> RBSRewriter::maybeInsertRBSCast(unique_ptr<parser::Node> node) {
//     unique_ptr<parser::Node> result;

//     if (auto klass = parser::cast_node<parser::Class>(node.get())) {
//         result = move(node);
//     } else if (auto module = parser::cast_node<parser::Module>(node.get())) {
//         result = move(node);
//     } else if (auto sclass = parser::cast_node<parser::SClass>(node.get())) {
//         result = move(node);
//     } else if (auto const_ = parser::cast_node<parser::Const>(node.get())) {
//         result = move(node);
//     } else if (auto asgn = parser::cast_node<parser::Assign>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(asgn->rhs, asgn->lhs->loc)) {
//             asgn->rhs = insertRBSCast(move(asgn->rhs), move(rbsType->first), rbsType->second);
//         }
//         result = move(node);
//     } else if (auto asgn = parser::cast_node<parser::AndAsgn>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(asgn->right, asgn->left->loc)) {
//             asgn->right = insertRBSCast(move(asgn->right), move(rbsType->first), rbsType->second);
//         }
//         result = move(node);
//     } else if (auto asgn = parser::cast_node<parser::OpAsgn>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(asgn->right, asgn->left->loc)) {
//             asgn->right = insertRBSCast(move(asgn->right), move(rbsType->first), rbsType->second);
//         }
//         result = move(node);
//     } else if (auto asgn = parser::cast_node<parser::OrAsgn>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(asgn->right, asgn->left->loc)) {
//             asgn->right = insertRBSCast(move(asgn->right), move(rbsType->first), rbsType->second);
//         }
//         result = move(node);
//     } else if (auto asgn = parser::cast_node<parser::Masgn>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(asgn->rhs, asgn->lhs->loc)) {
//             asgn->rhs = insertRBSCast(move(asgn->rhs), move(rbsType->first), rbsType->second);
//         }
//         result = move(node);
//     } else if (auto ret = parser::cast_node<parser::Return>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(node, ret->loc)) {
//             for (auto &expr : ret->exprs) {
//                 expr = insertRBSCast(move(expr), move(rbsType->first), rbsType->second);
//             }
//         }
//         result = move(node);
//     } else if (auto br = parser::cast_node<parser::Break>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(node, br->loc)) {
//             for (auto &expr : br->exprs) {
//                 expr = insertRBSCast(move(expr), move(rbsType->first), rbsType->second);
//             }
//         }
//         result = move(node);
//     } else if (auto next = parser::cast_node<parser::Next>(node.get())) {
//         if (auto rbsType = getRBSAssertionType(node, next->loc)) {
//             for (auto &expr : next->exprs) {
//                 expr = insertRBSCast(move(expr), move(rbsType->first), rbsType->second);
//             }
//         }
//         result = move(node);
//     } else {
//         if (auto rbsType = getRBSAssertionType(node, node->loc)) {
//             result = insertRBSCast(move(node), move(rbsType->first), rbsType->second);
//         } else {
//             result = move(node);
//         }
//     }

//     return result;
// }

unique_ptr<parser::Node> RBSRewriter::maybeInsertCast(unique_ptr<parser::Node> node) {
    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        [&](parser::Assign *asgn) {
            if (auto type = assertionForNode(asgn->rhs, asgn->lhs->loc)) {
                asgn->rhs = insertCast(move(asgn->rhs), move(type));
            }
            result = move(node);
        },
        [&](parser::AndAsgn *andAsgn) {
            if (auto type = assertionForNode(andAsgn->right, andAsgn->left->loc)) {
                andAsgn->right = insertCast(move(andAsgn->right), move(type));
            }
            result = move(node);
        },
        [&](parser::OpAsgn *opAsgn) {
            if (auto type = assertionForNode(opAsgn->right, opAsgn->left->loc)) {
                opAsgn->right = insertCast(move(opAsgn->right), move(type));
            }
            result = move(node);
        },
        [&](parser::OrAsgn *orAsgn) {
            if (auto type = assertionForNode(orAsgn->right, orAsgn->left->loc)) {
                orAsgn->right = insertCast(move(orAsgn->right), move(type));
            }
            result = move(node);
        },
        [&](parser::Masgn *masgn) {
            if (auto type = assertionForNode(masgn->rhs, masgn->lhs->loc)) {
                masgn->rhs = insertCast(move(masgn->rhs), move(type));
            }
            result = move(node);
        },

        [&](parser::Node *other) { result = move(node); });

    return result;
}

parser::NodeVec RBSRewriter::rewriteNodes(parser::NodeVec nodes) {
    auto oldStmts = move(nodes);
    auto newStmts = parser::NodeVec();

    for (auto &node : oldStmts) {
        newStmts.emplace_back(rewriteNode(move(node)));
    }

    return newStmts;
}

unique_ptr<parser::Node> RBSRewriter::rewriteBegin(unique_ptr<parser::Node> node) {
    auto begin = parser::cast_node<parser::Begin>(node.get());
    ENFORCE(begin != nullptr);

    auto oldStmts = move(begin->stmts);
    begin->stmts = parser::NodeVec();

    for (auto &stmt : oldStmts) {
        stmt = maybeInsertCast(move(stmt));
        begin->stmts.emplace_back(rewriteNode(move(stmt)));
    }

    return node;
}

unique_ptr<parser::Node> RBSRewriter::rewriteBody(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    if (auto begin = parser::cast_node<parser::Begin>(node.get())) {
        return rewriteBegin(move(node));
    }

    node = maybeInsertCast(move(node));
    return rewriteNode(move(node));
}

unique_ptr<parser::Node> RBSRewriter::rewriteNode(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        // Nodes are ordered as in desugar
        [&](parser::Send *send) {
            send->receiver = rewriteNode(move(send->receiver));
            send->args = rewriteNodes(move(send->args));
            result = move(node);
        },
        [&](parser::Hash *hash) {
            hash->pairs = rewriteNodes(move(hash->pairs));
            result = move(node);
        },
        [&](parser::Block *block) {
            maybeSaveSignature(block);
            block->body = rewriteBody(move(block->body));
            result = move(node);
        },
        [&](parser::Begin *begin) {
            //
            result = rewriteBegin(move(node));
        },
        [&](parser::Assign *asgn) {
            asgn->rhs = rewriteNode(move(asgn->rhs));
            result = move(node);
        },
        // END hand-ordered clauses
        [&](parser::And *and_) {
            and_->left = rewriteNode(move(and_->left));
            and_->right = rewriteNode(move(and_->right));
            result = move(node);
        },
        [&](parser::Or *or_) {
            or_->left = rewriteNode(move(or_->left));
            or_->right = rewriteNode(move(or_->right));
            result = move(node);
        },
        [&](parser::AndAsgn *andAsgn) {
            andAsgn->right = rewriteNode(move(andAsgn->right));
            result = move(node);
        },
        [&](parser::OrAsgn *orAsgn) {
            orAsgn->right = rewriteNode(move(orAsgn->right));
            result = move(node);
        },
        [&](parser::OpAsgn *opAsgn) {
            opAsgn->right = rewriteNode(move(opAsgn->right));
            result = move(node);
        },
        [&](parser::CSend *csend) {
            csend->receiver = rewriteNode(move(csend->receiver));
            csend->args = rewriteNodes(move(csend->args));
            result = move(node);
        },
        [&](parser::Kwbegin *kwbegin) {
            kwbegin->stmts = rewriteNodes(move(kwbegin->stmts));
            result = move(node);
        },
        [&](parser::Module *module) {
            module->body = rewriteBody(move(module->body));

            // if (auto assertion = findRBSTrailingCommentFromPos(module->loc.endPos())) {
            //     if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                 core::errors::Rewriter::RBSAssertionError)) {
            //         e.setHeader("Unexpected RBS assertion comment found after `{}` end", "module");
            //     }
            // }

            // auto decLine = core::Loc::pos2Detail(ctx.file.data(ctx), module->declLoc.endLoc).line;
            // auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), module->loc.endLoc).line;
            // if ((endLine > decLine)) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(module->declLoc.endLoc)) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found after `{}` declaration", "module");
            //         }
            //     }
            // }

            result = move(node);
        },
        [&](parser::Class *klass) {
            klass->body = rewriteBody(move(klass->body));

            // if (auto assertion = findRBSTrailingCommentFromPos(klass->loc.endPos())) {
            //     if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                 core::errors::Rewriter::RBSAssertionError)) {
            //         e.setHeader("Unexpected RBS assertion comment found after `{}` end", "class");
            //     }
            // }

            // auto decLine = core::Loc::pos2Detail(ctx.file.data(ctx), klass->declLoc.endLoc).line;
            // auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), klass->loc.endLoc).line;
            // if (endLine > decLine) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(klass->declLoc.endLoc)) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found after `{}` declaration", "class");
            //         }
            //     }
            // }

            result = move(node);
        },
        // [&](parser::Args *args) {
        //     args->args = rewriteNodes(move(args->args));
        //     result = move(node);
        // },
        // [&](parser::Arg *arg) { result = move(node); }, [&](parser::Restarg *arg) { result = move(node); },
        // [&](parser::Kwrestarg *arg) { result = move(node); }, [&](parser::Kwarg *arg) { result = move(node); },
        // [&](parser::Blockarg *arg) { result = move(node); }, [&](parser::Kwoptarg *arg) { result = move(node); },
        // [&](parser::Optarg *arg) { result = move(node); }, [&](parser::Shadowarg *arg) { result = move(node); },
        [&](parser::DefMethod *method) {
            // method->args = rewriteNode(move(method->args));
            method->body = rewriteBody(move(method->body));

            // if (auto assertion = findRBSTrailingCommentFromPos(method->loc.endPos())) {
            //     if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                 core::errors::Rewriter::RBSAssertionError)) {
            //         e.setHeader("Unexpected RBS assertion comment found after `{}` end", "method");
            //     }
            // }

            // auto decLine = core::Loc::pos2Detail(ctx.file.data(ctx), method->declLoc.endLoc).line;
            // auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), method->loc.endLoc).line;
            // if (endLine > decLine) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(method->declLoc.endLoc)) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found after `{}` declaration", "method");
            //         }
            //     }
            // }

            result = move(node);
        },
        [&](parser::DefS *method) {
            // method->args = rewriteNode(move(method->args));
            method->body = rewriteBody(move(method->body));

            // if (auto assertion = findRBSTrailingCommentFromPos(method->loc.endPos())) {
            //     if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                 core::errors::Rewriter::RBSAssertionError)) {
            //         e.setHeader("Unexpected RBS assertion comment found after `{}` end", "method");
            //     }
            // }

            // auto decLine = core::Loc::pos2Detail(ctx.file.data(ctx), method->declLoc.endLoc).line;
            // auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), method->loc.endLoc).line;
            // if (endLine > decLine) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(method->declLoc.endLoc)) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found after `{}` declaration", "method");
            //         }
            //     }
            // }

            result = move(node);
        },
        [&](parser::SClass *sclass) {
            sclass->body = rewriteBody(move(sclass->body));
            result = move(node);
        },
        [&](parser::When *when) {
            when->body = rewriteBody(move(when->body));
            result = move(node);
        },
        [&](parser::While *wl) {
            wl->body = rewriteBody(move(wl->body));
            result = move(node);
        },
        [&](parser::WhilePost *wl) {
            wl->body = rewriteBody(move(wl->body));
            result = move(node);
        },
        [&](parser::Until *until) {
            until->body = rewriteBody(move(until->body));
            result = move(node);
        },
        [&](parser::UntilPost *until) {
            until->body = rewriteBody(move(until->body));
            result = move(node);
        },
        [&](parser::For *for_) {
            for_->body = rewriteBody(move(for_->body));
            result = move(node);
        },
        [&](parser::Return *ret) {
            // if (ret->exprs.empty()) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(ret->loc.endPos())) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found in `{}` without an expression",
            //             "return");
            //         }
            //     }
            // }
            ret->exprs = rewriteNodes(move(ret->exprs));
            result = move(node);
        },
        [&](parser::Break *break_) {
            // if (break_->exprs.empty()) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(break_->loc.endPos())) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found in `{}` without an expression",
            //             "break");
            //         }
            //     }
            // }
            break_->exprs = rewriteNodes(move(break_->exprs));
            result = move(node);
        },
        [&](parser::Next *next) {
            // if (next->exprs.empty()) {
            //     if (auto assertion = findRBSTrailingCommentFromPos(next->loc.endPos())) {
            //         if (auto e = ctx.beginError(assertion.value().comment.commentLoc,
            //                                     core::errors::Rewriter::RBSAssertionError)) {
            //             e.setHeader("Unexpected RBS assertion comment found in `{}` without an expression",
            //             "next");
            //         }
            //     }
            // }
            next->exprs = rewriteNodes(move(next->exprs));
            result = move(node);
        },
        [&](parser::Rescue *rescue) {
            rescue->body = rewriteBody(move(rescue->body));
            rescue->else_ = rewriteBody(move(rescue->else_));
            result = move(node);
        },
        [&](parser::Resbody *resbody) {
            resbody->body = rewriteBody(move(resbody->body));
            result = move(node);
        },
        [&](parser::Ensure *ensure) {
            ensure->body = rewriteBody(move(ensure->body));
            result = move(node);
        },
        [&](parser::If *if_) {
            if_->then_ = rewriteBody(move(if_->then_));
            if_->else_ = rewriteBody(move(if_->else_));
            result = move(node);
        },
        [&](parser::Masgn *masgn) {
            masgn->rhs = rewriteNode(move(masgn->rhs));
            result = move(node);
        },
        [&](parser::Case *case_) {
            case_->whens = rewriteNodes(move(case_->whens));
            case_->else_ = rewriteBody(move(case_->else_));
            result = move(node);
        },
        [&](parser::Node *other) { result = move(node); });

    return result;
}

unique_ptr<parser::Node> RBSRewriter::run(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    return rewriteBody(move(node));
}

} // namespace sorbet::rbs
