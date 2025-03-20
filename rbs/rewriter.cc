#include "rbs/rewriter.h"

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

/**
 * A collection of annotations and signatures comments found on a method definition.
 */
struct Comments {
    /**
     * RBS annotation comments found on a method definition.
     *
     * Annotations are formatted as `@some_annotation`.
     */
    vector<rbs::Comment> annotations;

    /**
     * RBS signature comments found on a method definition.
     *
     * Signatures are formatted as `#: () -> void`.
     */
    vector<rbs::Comment> signatures;
};

Comments findRBSSignatureComments(string_view sourceCode, core::LocOffsets loc) {
    vector<rbs::Comment> annotations;
    vector<rbs::Comment> signatures;

    uint32_t beginIndex = loc.beginPos();

    // Everything in the file before the method definition
    string_view preDefinition = sourceCode.substr(0, sourceCode.rfind('\n', beginIndex));

    // Get all the lines before it
    vector<string_view> all_lines = absl::StrSplit(preDefinition, '\n');

    // We compute the current position in the source so we know the location of each comment
    uint32_t index = beginIndex;

    // NOTE: This is accidentally quadratic.
    // Instead of looping over all the lines between here and the start of the file, we should
    // instead track something like the locs of all the expressions in the ClassDef::rhs, and
    // only scan over the space between the ClassDef::rhs top level items

    // Iterate from the last line, to the first line
    for (auto it = all_lines.rbegin(); it != all_lines.rend(); it++) {
        index -= it->size();
        index -= 1;

        string_view line = absl::StripAsciiWhitespace(*it);

        // Short circuit when line is empty
        if (line.empty()) {
            break;
        }

        // Handle single-line sig block
        else if (absl::StartsWith(line, "sig")) {
            // Do nothing for a one-line sig block
            // TODO: Handle single-line sig blocks
        }

        // Handle multi-line sig block
        else if (absl::StartsWith(line, "end")) {
            // ASSUMPTION: We either hit the start of file, a `sig do`/`sig(:final) do` or an `end`
            // TODO: Handle multi-line sig blocks
            it++;
            while (
                // SOF
                it != all_lines.rend()
                // Start of sig block
                && !(absl::StartsWith(absl::StripAsciiWhitespace(*it), "sig do") ||
                     absl::StartsWith(absl::StripAsciiWhitespace(*it), "sig(:final) do"))
                // Invalid end keyword
                && !absl::StartsWith(absl::StripAsciiWhitespace(*it), "end")) {
                it++;
            };

            // We have either
            // 1) Reached the start of the file
            // 2) Found a `sig do`
            // 3) Found an invalid end keyword
            if (it == all_lines.rend() || absl::StartsWith(absl::StripAsciiWhitespace(*it), "end")) {
                break;
            }

            // Reached a sig block.
            line = absl::StripAsciiWhitespace(*it);
            ENFORCE(absl::StartsWith(line, "sig do") || absl::StartsWith(line, "sig(:final) do"));

            // Stop looking if this is a single-line block e.g `sig do; <block>; end`
            if ((absl::StartsWith(line, "sig do;") || absl::StartsWith(line, "sig(:final) do;")) &&
                absl::EndsWith(line, "end")) {
                break;
            }

            // Else, this is a valid sig block. Move on to any possible documentation.
        }

        // Handle a RBS sig annotation `#: SomeRBS`
        else if (absl::StartsWith(line, "#:")) {
            // Account for whitespace before the annotation e.g
            // #: abc -> "abc"
            // #:abc -> "abc"
            int lineSize = line.size();
            auto rbsSignature = rbs::Comment{
                core::LocOffsets{index, index + lineSize},
                line.substr(2),
            };
            signatures.emplace_back(rbsSignature);
        }

        // Handle RDoc annotations `# @abstract`
        else if (absl::StartsWith(line, "# @")) {
            int lineSize = line.size();
            auto annotation = rbs::Comment{
                core::LocOffsets{index, index + lineSize},
                line.substr(3),
            };
            annotations.emplace_back(annotation);
        }

        // Ignore other comments
        else if (absl::StartsWith(line, "#")) {
            continue;
        }

        // No other cases applied to this line, so stop looking.
        else {
            break;
        }
    }

    reverse(annotations.begin(), annotations.end());
    reverse(signatures.begin(), signatures.end());

    return Comments{annotations, signatures};
}

unique_ptr<parser::NodeVec> getRBSSignatures(core::MutableContext ctx, unique_ptr<parser::Node> &node) {
    auto methodComments = findRBSSignatureComments(ctx.file.data(ctx).source(), node->loc);

    if (methodComments.signatures.empty()) {
        return nullptr;
    }

    auto signatures = make_unique<parser::NodeVec>();

    for (auto &signature : methodComments.signatures) {
        auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, signature);
        if (rbsMethodType.first) {
            unique_ptr<parser::Node> sig;
            if (parser::isa_node<parser::DefMethod>(node.get()) || parser::isa_node<parser::DefS>(node.get())) {
                sig = rbs::MethodTypeTranslator::methodSignatureNode(ctx, node.get(), move(rbsMethodType.first.value()),
                                                                     methodComments.annotations);
            } else {
                Exception::raise("Unimplemented node type: {}", node->nodeName());
            }
            signatures->emplace_back(move(sig));
        } else {
            ENFORCE(rbsMethodType.second);
            if (auto e = ctx.beginError(rbsMethodType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                e.setHeader("Failed to parse RBS signature ({})", rbsMethodType.second->message);
            }
        }
    }
    return signatures;
}

/**
 * Check if the given range contains a heredoc marker `<<-` or `<<~`
 */
bool hasHeredocMarker(core::MutableContext ctx, const uint32_t fromPos, const uint32_t toPos) {
    string source(ctx.file.data(ctx).source().substr(fromPos, toPos - fromPos));
    regex heredoc_pattern("(\\s+=\\s<<-|~)");
    smatch matches;
    return regex_search(source, matches, heredoc_pattern);
}

/**
 * Check if the given expression is a heredoc
 */
bool isHeredoc(core::MutableContext ctx, core::LocOffsets assignLoc, const unique_ptr<parser::Node> &node) {
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
                if (hasHeredocMarker(ctx, assignLoc.endPos(), lit->loc.beginPos())) {
                    result = true;
                }
            } else {
                // Multi-line heredoc, we look for the heredoc marker inside the string itself.
                if (hasHeredocMarker(ctx, lit->loc.beginPos(), lit->loc.endPos())) {
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
                if (hasHeredocMarker(ctx, assignLoc.endPos(), lit->loc.beginPos())) {
                    result = true;
                }
            } else {
                // Multi-line heredoc, we look for the heredoc marker inside the string itself.
                if (hasHeredocMarker(ctx, lit->loc.beginPos(), lit->loc.endPos())) {
                    result = true;
                }
            }
        },
        [&](parser::Send *send) {
            result = isHeredoc(ctx, assignLoc, send->receiver) ||
                     absl::c_any_of(send->args, [&](const unique_ptr<parser::Node> &arg) {
                         return isHeredoc(ctx, assignLoc, arg);
                     });
        },
        [&](parser::Node *expr) { result = false; });

    return result;
}

optional<rbs::Comment> findRBSTrailingComment(core::MutableContext ctx, unique_ptr<parser::Node> &node,
                                              core::LocOffsets fromLoc) {
    // std::cerr << "Finding RBS comments for node: " << node->toString(ctx) << std::endl;

    auto source = ctx.file.data(ctx).source();

    // We want to find the comment right after the end of the assign
    auto startingLoc = node->loc.endPos();

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
    if (isHeredoc(ctx, fromLoc, node)) {
        startingLoc = fromLoc.beginPos();
    }

    // Get the position of the end of the line from the startingLoc
    auto endOfLine = source.find('\n', startingLoc);
    if (endOfLine == string::npos) {
        return nullopt;
    }

    // Check between the startingLoc and the end of the line for a `#: ...` comment
    auto comment = source.substr(startingLoc, endOfLine - startingLoc);

    // Find the position of the `#:` in the comment
    auto commentStart = comment.find("#:");
    if (commentStart == string::npos) {
        return nullopt;
    }

    // Adjust the location to be the correct position depending on the number of spaces after the `#:`
    auto offset = 0;
    for (auto i = startingLoc + commentStart + 2; i < endOfLine; i++) {
        if (source[i] == ' ') {
            offset++;
        } else {
            break;
        }
    }

    return rbs::Comment{
        core::LocOffsets{startingLoc + (uint32_t)commentStart + offset, static_cast<uint32_t>(endOfLine)},
        absl::StripAsciiWhitespace(comment.substr(commentStart + 2))};
}

/**
 * Get the RBS type from the given assign
 */
unique_ptr<parser::Node> getRBSAssertionType(core::MutableContext ctx, unique_ptr<parser::Node> &node,
                                             core::LocOffsets fromLoc) {
    auto assertion = findRBSTrailingComment(ctx, node, fromLoc);

    if (!assertion) {
        return nullptr;
    }

    auto result = rbs::RBSParser::parseType(ctx, *assertion);
    if (result.second) {
        if (auto e = ctx.beginError(result.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type ({})", result.second->message);
        }
        return nullptr;
    }

    auto rbsType = move(result.first.value());
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>();
    return rbs::TypeTranslator::toParserNode(ctx, typeParams, rbsType.node.get(), assertion->loc);
}

parser::NodeVec rewriteNodes(core::MutableContext ctx, parser::NodeVec nodes) {
    std::cerr << "Rewriting nodes" << std::endl;

    auto oldStmts = move(nodes);
    auto newStmts = parser::NodeVec();

    for (auto &node : oldStmts) {
        newStmts.emplace_back(rewriteNode(ctx, move(node)));
    }

    return newStmts;
}

unique_ptr<parser::Node> rewriteBegin(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    std::cerr << "Rewriting begin" << std::endl;

    auto begin = parser::cast_node<parser::Begin>(node.get());
    auto oldStmts = move(begin->stmts);
    auto newStmts = parser::NodeVec();

    for (auto &stmt : oldStmts) {
        if (auto def = parser::cast_node<parser::DefMethod>(stmt.get())) {
            if (auto comments = getRBSSignatures(ctx, stmt)) {
                for (auto &signature : *comments) {
                    newStmts.emplace_back(move(signature));
                }
            }
        } else if (auto def = parser::cast_node<parser::DefS>(stmt.get())) {
            if (auto comments = getRBSSignatures(ctx, stmt)) {
                for (auto &signature : *comments) {
                    newStmts.emplace_back(move(signature));
                }
            }
        }

        newStmts.emplace_back(rewriteNode(ctx, move(stmt)));
    }

    begin->stmts = move(newStmts);
    return node;
}

unique_ptr<parser::Node> rewriteBody(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    std::cerr << "Rewriting body" << std::endl;
    if (auto begin = parser::cast_node<parser::Begin>(node.get())) {
        return rewriteBegin(ctx, move(node));
    } else if (auto def = parser::cast_node<parser::DefMethod>(node.get())) {
        if (auto comments = getRBSSignatures(ctx, node)) {
            auto newStmts = parser::NodeVec();
            for (auto &signature : *comments) {
                newStmts.emplace_back(move(signature));
            }
            newStmts.emplace_back(rewriteNode(ctx, move(node)));
            return make_unique<parser::Begin>(def->loc, move(newStmts));
        }
    } else if (auto def = parser::cast_node<parser::DefS>(node.get())) {
        std::cerr << "Rewriting defs" << std::endl;
        if (auto comments = getRBSSignatures(ctx, node)) {
            std::cerr << "Got comments: " << comments->size() << std::endl;
            auto newStmts = parser::NodeVec();
            for (auto &signature : *comments) {
                std::cerr << "Adding signature: " << std::endl;
                newStmts.emplace_back(move(signature));
            }
            std::cerr << "Adding node: " << std::endl;
            newStmts.emplace_back(rewriteNode(ctx, move(node)));
            return make_unique<parser::Begin>(def->loc, move(newStmts));
        }
    }

    return rewriteNode(ctx, move(node));
}

}; // namespace

unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        // Nodes are ordered as in desugar
        [&](parser::Const *const_) { result = move(node); },
        [&](parser::Send *send) {
            send->receiver = rewriteNode(ctx, move(send->receiver));
            send->args = rewriteNodes(ctx, move(send->args));
            result = move(node);
        },
        [&](parser::String *string) { result = move(node); }, [&](parser::Symbol *symbol) { result = move(node); },
        [&](parser::LVar *var) { result = std::move(node); }, [&](parser::Hash *hash) { result = move(node); },
        [&](parser::Block *block) {
            block->body = rewriteNode(ctx, move(block->body));
            result = move(node);
        },
        [&](parser::Begin *begin) { result = rewriteBegin(ctx, move(node)); },
        [&](parser::Assign *asgn) {
            if (auto rbsType = getRBSAssertionType(ctx, asgn->rhs, asgn->lhs->loc)) {
                auto rhs = rewriteNode(ctx, move(asgn->rhs));
                asgn->rhs = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        // END hand-ordered clauses
        [&](parser::And *and_) {
            and_->left = rewriteNode(ctx, move(and_->left));
            and_->right = rewriteNode(ctx, move(and_->right));
            result = move(node);
        },
        [&](parser::Or *or_) {
            or_->left = rewriteNode(ctx, move(or_->left));
            or_->right = rewriteNode(ctx, move(or_->right));
            result = move(node);
        },
        [&](parser::AndAsgn *andAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, andAsgn->right, andAsgn->left->loc)) {
                auto rhs = rewriteNode(ctx, move(andAsgn->right));
                andAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::OrAsgn *orAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, orAsgn->right, orAsgn->left->loc)) {
                auto rhs = rewriteNode(ctx, move(orAsgn->right));
                orAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::OpAsgn *opAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, opAsgn->right, opAsgn->left->loc)) {
                auto rhs = rewriteNode(ctx, move(opAsgn->right));
                opAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::CSend *csend) {
            csend->receiver = rewriteNode(ctx, move(csend->receiver));
            csend->args = rewriteNodes(ctx, move(csend->args));
            result = move(node);
        },
        [&](parser::Self *self) { result = move(node); },
        //[&](parser::DSymbol *dsymbol) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::FileLiteral *fileLiteral) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        [&](parser::ConstLhs *constLhs) { result = move(node); },
        //[&](parser::Cbase *cbase) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwbegin *kwbegin) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Module *module) {
            module->body = rewriteBody(ctx, move(module->body));
            result = move(node);
        },
        [&](parser::Class *klass) {
            std::cerr << "Rewriting class: " << std::endl;
            klass->body = rewriteBody(ctx, move(klass->body));
            result = move(node);
        },
        // [&](parser::Args *args) {
        //     args->args = rewriteNodes(ctx, move(args->args));
        //     result = move(node);
        // },
        // [&](parser::Arg *arg) { result = move(node); }, [&](parser::Restarg *arg) { result = move(node); },
        // [&](parser::Kwrestarg *arg) { result = move(node); }, [&](parser::Kwarg *arg) { result = move(node); },
        // [&](parser::Blockarg *arg) { result = move(node); }, [&](parser::Kwoptarg *arg) { result = move(node); },
        // [&](parser::Optarg *arg) { result = move(node); }, [&](parser::Shadowarg *arg) { result = move(node); },
        [&](parser::DefMethod *method) {
            // method->args = rewriteNode(ctx, move(method->args));
            method->body = rewriteNode(ctx, move(method->body));
            result = move(node);
        },
        [&](parser::DefS *method) {
            // method->args = rewriteNode(ctx, move(method->args));
            method->body = rewriteNode(ctx, move(method->body));
            result = move(node);
        },
        [&](parser::SClass *sclass) {
            sclass->body = rewriteBody(ctx, move(sclass->body));
            result = move(node);
        },
        //[&](parser::NumBlock *block) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::While *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::WhilePost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Until *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::UntilPost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Nil *wl) { result = move(node); }, [&](parser::IVar *var) { result = move(node); },
        [&](parser::GVar *var) { result = move(node); }, [&](parser::CVar *var) { result = move(node); },
        [&](parser::LVarLhs *var) { result = move(node); }, [&](parser::GVarLhs *var) { result = move(node); },
        [&](parser::CVarLhs *var) { result = move(node); }, [&](parser::IVarLhs *var) { result = move(node); },
        //[&](parser::NthRef *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Super *super) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ZSuper *zuper) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::For *for_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Integer *integer) { result = move(node); }, [&](parser::DString *dstring) { result = move(node); },
        //[&](parser::Float *floatNode) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Complex *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Rational *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Array *array) { result = move(node); },
        //[&](parser::IRange *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ERange *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Regexp *regexpNode) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Regopt *regopt) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Return *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Break *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Next *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Retry *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Yield *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Rescue *rescue) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Resbody *resbody) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Ensure *ensure) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::If *if_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Masgn *masgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node, masgn->rhs->loc)) {
                auto rhs = rewriteNode(ctx, move(masgn->rhs));
                masgn->rhs = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        //[&](parser::True *t) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::False *t) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Case *case_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Splat *splat) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ForwardedRestArg *fra) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Alias *alias) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Defined *defined) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::LineLiteral *line) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::XString *xstring) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Preexe *preexe) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Postexe *postexe) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Undef *undef) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CaseMatch *caseMatch) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Backref *backref) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EFlipflop *eflipflop) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::IFlipflop *iflipflop) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::MatchCurLine *matchCurLine) {
        //    Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        //[&](parser::Redo *redo) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EncodingLiteral *encodingLiteral) {
        //    Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        //[&](parser::MatchPattern *pattern) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::MatchPatternP *pattern) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EmptyElse *else_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::BlockPass *blockPass) { Exception::raise("Send should have already handled the BlockPass"); },
        [&](parser::Node *other) {
            std::cerr << "Unimplemented Parser Node: " << node->nodeName() << std::endl;
            Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
            exit(1);
        });

    return result;
}

} // namespace sorbet::rbs
