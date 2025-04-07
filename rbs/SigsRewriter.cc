#include "rbs/SigsRewriter.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "common/typecase.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "rbs/SignatureTranslator.h"

using namespace std;

namespace sorbet::rbs {

namespace {

const string_view RBS_PREFIX = "#:";
const string_view ANNOTATION_PREFIX = "# @";

bool isVisibilitySend(const parser::Send *send) {
    return send->receiver == nullptr && send->args.size() == 1 &&
           (parser::isa_node<parser::DefMethod>(send->args[0].get()) ||
            parser::isa_node<parser::DefS>(send->args[0].get())) &&
           (send->method == core::Names::private_() || send->method == core::Names::protected_() ||
            send->method == core::Names::public_() || send->method == core::Names::privateClassMethod() ||
            send->method == core::Names::publicClassMethod() || send->method == core::Names::packagePrivate() ||
            send->method == core::Names::packagePrivateClassMethod());
}

bool isAttrAccessorSend(const parser::Send *send) {
    return (send->receiver == nullptr || parser::isa_node<parser::Self>(send->receiver.get())) &&
           (send->method == core::Names::attrReader() || send->method == core::Names::attrWriter() ||
            send->method == core::Names::attrAccessor());
}

parser::Node *signaturesTarget(parser::Node *node) {
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

Comment createComment(size_t start, size_t end, string_view prefix, string_view content) {
    auto start32 = (uint32_t)start;
    auto end32 = (uint32_t)end;
    auto commentLoc = core::LocOffsets{start32, end32};

    auto typeStart = start32 + (uint32_t)prefix.size();
    auto typeLoc = core::LocOffsets{typeStart, end32};

    return Comment{commentLoc, typeLoc, content};
}

Comments signaturesForLoc(core::MutableContext ctx, core::LocOffsets loc,
                          std::vector<std::pair<size_t, size_t>> &commentLocations) {
    Comments result;

    // Iterate in reverse to find the closest comment before the location
    auto it = commentLocations.rbegin();
    while (it != commentLocations.rend()) {
        auto start = it->first;
        auto end = it->second;
        auto nextIt = it + 1;

        if (end <= loc.beginPos()) {
            auto commentLength = end - start;
            auto commentText = ctx.file.data(ctx).source().substr(start, commentLength);

            if (absl::StartsWith(commentText, RBS_PREFIX)) {
                auto signature = commentText.substr(RBS_PREFIX.size());
                result.signatures.emplace_back(createComment(start, end, RBS_PREFIX, signature));
            } else if (absl::StartsWith(commentText, ANNOTATION_PREFIX)) {
                auto annotation = commentText.substr(ANNOTATION_PREFIX.size());
                result.annotations.emplace_back(createComment(start, end, ANNOTATION_PREFIX, annotation));
            }

            // Check if we have a comment on the previous line
            if (nextIt != commentLocations.rend()) {
                auto currentItLine = core::Loc::pos2Detail(ctx.file.data(ctx), start).line;
                auto nextItLine = core::Loc::pos2Detail(ctx.file.data(ctx), nextIt->second).line;
                if (currentItLine == nextItLine + 1) {
                    // Remove the processed comment
                    commentLocations.erase(it.base() - 1);
                    it = nextIt;

                    // There's a comment directly above the current comment, process it in the next iteration
                    continue;
                }
            }

            // Remove the processed comment
            commentLocations.erase(it.base() - 1);
            break; // We processed all comments for this node location
        }
        ++it;
    }

    return result;
}

unique_ptr<parser::NodeVec> signaturesForNode(core::MutableContext ctx, parser::Node *node,
                                              std::vector<std::pair<size_t, size_t>> &commentLocations) {
    auto comments = signaturesForLoc(ctx, node->loc, commentLocations);

    if (comments.signatures.empty()) {
        return nullptr;
    }

    auto signatures = make_unique<parser::NodeVec>();
    auto signatureTranslator = rbs::SignatureTranslator(ctx);

    for (auto &signature : comments.signatures) {
        if (parser::isa_node<parser::DefMethod>(node) || parser::isa_node<parser::DefS>(node)) {
            auto sig = signatureTranslator.translateSignature(node, signature, comments.annotations);

            signatures->emplace_back(move(sig));
        } else if (auto send = parser::cast_node<parser::Send>(node)) {
            auto sig = signatureTranslator.translateType(send, signature, comments.annotations);
            signatures->emplace_back(move(sig));
        } else {
            Exception::raise("Unimplemented node type: {}", node->nodeName());
        }
    }

    return signatures;
}

} // namespace

// @kaan False positives with YARD (@api) and inline assertions
void SigsRewriter::checkForUnusedComments() {
    // We erased comments as we processed them, anything left over is an error
    // for (const auto &loc : commentLocations) {
    //     auto start = static_cast<uint32_t>(loc.first);
    //     auto end = static_cast<uint32_t>(loc.second);
    //     auto commentText = ctx.file.data(ctx).source().substr(start, end - start);

    //     if (absl::StartsWith(commentText,
    //                          RBS_PREFIX)) { // @kaan ANNOTATION_PREFIX causes false positives with YARD (@api)
    //         if (auto e = ctx.beginError(core::LocOffsets{start, end}, core::errors::Rewriter::RBSUnusedComment)) {
    //             e.setHeader("Unused RBS signature comment. No method definition found after it");
    //         }
    //     }
    // }
}

parser::NodeVec SigsRewriter::rewriteNodes(parser::NodeVec nodes) {
    parser::NodeVec result;

    for (auto &node : nodes) {
        result.emplace_back(rewriteBody(move(node)));
    }

    return result;
}

unique_ptr<parser::Node> SigsRewriter::rewriteBegin(unique_ptr<parser::Node> node) {
    auto begin = parser::cast_node<parser::Begin>(node.get());
    ENFORCE(begin != nullptr);

    auto oldStmts = move(begin->stmts);
    begin->stmts = parser::NodeVec();

    for (auto &stmt : oldStmts) {
        if (auto target = signaturesTarget(stmt.get())) {
            if (auto signatures = signaturesForNode(ctx, target, commentLocations)) {
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

    if (parser::isa_node<parser::Begin>(node.get())) {
        return rewriteBegin(move(node));
    }

    if (auto target = signaturesTarget(node.get())) {
        if (auto signatures = signaturesForNode(ctx, target, commentLocations)) {
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
        // Using the same order as Desugar.cc
        [&](parser::Block *block) {
            block->body = rewriteBody(move(block->body));
            result = move(node);
        },
        [&](parser::Begin *begin) {
            node = rewriteBegin(move(node));
            result = move(node);
        },
        [&](parser::Assign *assign) {
            assign->rhs = rewriteNode(move(assign->rhs));
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
        [&](parser::Kwbegin *kwbegin) {
            kwbegin->stmts = rewriteNodes(move(kwbegin->stmts));
            result = move(node);
        },
        [&](parser::Module *module) {
            module->body = rewriteBody(move(module->body));
            result = move(node);
        },
        [&](parser::Class *klass) {
            klass->body = rewriteBody(move(klass->body));
            result = move(node);
        },
        [&](parser::DefMethod *def) {
            def->body = rewriteBody(move(def->body));
            result = move(node);
        },
        [&](parser::DefS *def) {
            def->body = rewriteBody(move(def->body));
            result = move(node);
        },
        [&](parser::SClass *sclass) {
            sclass->body = rewriteBody(move(sclass->body));
            result = move(node);
        },
        [&](parser::For *for_) {
            for_->body = rewriteBody(move(for_->body));
            result = move(node);
        },
        [&](parser::Array *array) {
            array->elts = rewriteNodes(move(array->elts));
            result = move(node);
        },
        [&](parser::Rescue *rescue) {
            rescue->body = rewriteBody(move(rescue->body));
            rescue->rescue = rewriteNodes(move(rescue->rescue));
            rescue->else_ = rewriteBody(move(rescue->else_));
            result = move(node);
        },
        [&](parser::Resbody *resbody) {
            resbody->body = rewriteBody(move(resbody->body));
            result = move(node);
        },
        [&](parser::Ensure *ensure) {
            ensure->body = rewriteBody(move(ensure->body));
            ensure->ensure = rewriteBody(move(ensure->ensure));
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
        [&](parser::When *when) {
            when->body = rewriteBody(move(when->body));
            result = move(node);
        },
        [&](parser::Node *other) { result = move(node); });

    return result;
}

unique_ptr<parser::Node> SigsRewriter::run(unique_ptr<parser::Node> node) {
    auto body = rewriteBody(move(node));
    checkForUnusedComments();
    return body;
}

} // namespace sorbet::rbs
