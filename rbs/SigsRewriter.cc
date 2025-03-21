#include "rbs/SigsRewriter.h"

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

bool isVisibilitySend(parser::Send *send) {
    return send->receiver == nullptr && send->args.size() == 1 &&
           (parser::isa_node<parser::DefMethod>(send->args[0].get()) ||
            parser::isa_node<parser::DefS>(send->args[0].get())) &&
           (send->method == core::Names::private_() || send->method == core::Names::protected_() ||
            send->method == core::Names::public_() || send->method == core::Names::privateClassMethod() ||
            send->method == core::Names::publicClassMethod() || send->method == core::Names::packagePrivate() ||
            send->method == core::Names::packagePrivateClassMethod());
}

bool isAttrAccessorSend(parser::Send *send) {
    return send->receiver == nullptr &&
           (send->method == core::Names::attrReader() || send->method == core::Names::attrWriter() ||
            send->method == core::Names::attrAccessor());
}

parser::Node *signatureTarget(parser::Node *node) {
    if (parser::isa_node<parser::DefMethod>(node) || parser::cast_node<parser::DefS>(node)) {
        return node;
    }

    if (auto send = parser::cast_node<parser::Send>(node)) {
        if (isVisibilitySend(send)) {
            return send->args[0].get();
        } else if (isAttrAccessorSend(send)) {
            return node;
        }
    }

    return nullptr;
}

} // namespace

Comments SigsRewriter::signaturesForLoc(core::LocOffsets loc) {
    auto source = ctx.file.data(ctx).source();

    vector<rbs::Comment> annotations;
    vector<rbs::Comment> signatures;

    uint32_t beginIndex = loc.beginPos();

    // Everything in the file before the method definition
    string_view preDefinition = source.substr(0, source.rfind('\n', beginIndex));

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
                core::LocOffsets{index + 2, index + lineSize},
                line.substr(2),
            };
            signatures.emplace_back(rbsSignature);
        }

        // Handle RDoc annotations `# @abstract`
        else if (absl::StartsWith(line, "# @")) {
            int lineSize = line.size();
            auto annotation = rbs::Comment{
                core::LocOffsets{index, index + lineSize},
                core::LocOffsets{index + 3, index + lineSize},
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

unique_ptr<parser::NodeVec> SigsRewriter::signaturesForNode(parser::Node *node) {
    auto comments = signaturesForLoc(node->loc);

    if (comments.signatures.empty()) {
        return nullptr;
    }

    auto signatures = make_unique<parser::NodeVec>();

    for (auto &signature : comments.signatures) {
        if (parser::isa_node<parser::DefMethod>(node) || parser::isa_node<parser::DefS>(node)) {
            auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, signature);
            if (rbsMethodType.first) {
                unique_ptr<parser::Node> sig;

                sig = rbs::MethodTypeTranslator::methodSignature(
                    ctx, node, signature.commentLoc, move(rbsMethodType.first.value()), comments.annotations);

                signatures->emplace_back(move(sig));
            } else {
                ENFORCE(rbsMethodType.second);
                if (auto e = ctx.beginError(rbsMethodType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                    e.setHeader("Failed to parse RBS signature ({})", rbsMethodType.second->message);
                }
            }
        } else if (auto send = parser::cast_node<parser::Send>(node)) {
            auto rbsType = rbs::RBSParser::parseType(ctx, signature);
            if (rbsType.first) {
                auto sig = rbs::MethodTypeTranslator::attrSignature(ctx, send, signature.commentLoc,
                                                                    move(rbsType.first.value()), comments.annotations);
                signatures->emplace_back(move(sig));
            } else {
                ENFORCE(rbsType.second);

                // Before raising a parse error, let's check if the user tried to use a method signature on an
                // accessor
                auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, signature);
                if (rbsMethodType.first) {
                    if (auto e = ctx.beginError(rbsType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                        e.setHeader("Using a method signature on an accessor is not allowed, use a bare type instead");
                    }
                } else {
                    if (auto e = ctx.beginError(rbsType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                        e.setHeader("Failed to parse RBS type ({})", rbsType.second->message);
                    }
                }
            }
        } else {
            Exception::raise("Unimplemented node type: {}", node->nodeName());
        }
    }
    return signatures;
}

unique_ptr<parser::Node> SigsRewriter::rewriteBegin(unique_ptr<parser::Node> node) {
    auto begin = parser::cast_node<parser::Begin>(node.get());
    ENFORCE(begin != nullptr);

    auto oldStmts = move(begin->stmts);
    begin->stmts = parser::NodeVec();

    for (auto &stmt : oldStmts) {
        if (auto target = signatureTarget(stmt.get())) {
            if (auto signatures = signaturesForNode(target)) {
                for (auto &signature : *signatures) {
                    begin->stmts.emplace_back(move(signature));
                }
            }
        }

        begin->stmts.emplace_back(rewriteNode(move(stmt)));
    }

    return node;
}

unique_ptr<parser::Node> SigsRewriter::rewriteBody(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    if (auto begin = parser::cast_node<parser::Begin>(node.get())) {
        return rewriteBegin(move(node));
    }

    if (auto target = signatureTarget(node.get())) {
        if (auto signatures = signaturesForNode(target)) {
            auto begin = make_unique<parser::Begin>(node->loc, parser::NodeVec());
            for (auto &signature : *signatures) {
                begin->stmts.emplace_back(move(signature));
            }
            begin->stmts.emplace_back(move(node));
            return move(begin);
        }
    }

    return rewriteNode(move(node));
}

unique_ptr<parser::Node> SigsRewriter::rewriteNode(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        [&](parser::Module *module) {
            module->body = rewriteBody(move(module->body));
            result = move(node);
        },
        [&](parser::Class *klass) {
            klass->body = rewriteBody(move(klass->body));
            result = move(node);
        },
        [&](parser::SClass *sclass) {
            sclass->body = rewriteBody(move(sclass->body));
            result = move(node);
        },
        [&](parser::Block *block) {
            block->body = rewriteBody(move(block->body));
            result = move(node);
        },
        [&](parser::Begin *begin) {
            node = rewriteBegin(move(node));
            result = move(node);
        },
        [&](parser::Node *other) { result = move(node); });

    return result;
}

unique_ptr<parser::Node> SigsRewriter::run(unique_ptr<parser::Node> node) {
    return rewriteBody(move(node));
}

} // namespace sorbet::rbs
