#include "rbs/SigsRewriter.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "common/typecase.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "rbs/SignatureTranslator.h"
#include "rbs/TypeParamsToParserNodes.h"

using namespace std;

namespace sorbet::rbs {

namespace {

parser::Node *signaturesTarget(parser::Node *node) {
    if (parser::isa_node<parser::DefMethod>(node) || parser::cast_node<parser::DefS>(node)) {
        return node;
    }

    if (auto send = parser::cast_node<parser::Send>(node)) {
        if (parser::MK::isAttrAccessorSend(send) || parser::MK::isMethodDefModifierSend(send)) {
            return node;
        }
    }

    return nullptr;
}

/**
 * Extracts and parses the argument from the annotation string.
 *
 * Considering an annotation like `@mixes_in_class_methods: ClassMethods`,
 * this function will extract and parse `ClassMethods` as a type then return the corresponding parser::Node.
 *
 * We do not error if the node is not a constant, we just insert it as is and let the pipeline error down the line.
 */
unique_ptr<parser::Node> extractHelperArgument(core::MutableContext ctx, Comment annotation, int offset) {
    while (annotation.string[offset] == ' ') {
        offset++;
    }

    Comment comment = {
        core::LocOffsets{annotation.typeLoc.beginPos() + offset, annotation.typeLoc.endPos()},
        core::LocOffsets{annotation.typeLoc.beginPos() + offset, annotation.typeLoc.endPos()},
        annotation.string.substr(offset),
    };

    return rbs::SignatureTranslator(ctx).translateType(RBSDeclaration{{comment}});
}

/**
 * Extracts and parses the helpers from the annotations.
 *
 * Returns a `parser::NodeVec`, containing the `parser::Node` corresponding to each annotation.
 *
 * For example, given the annotations the following annotations:
 *
 *     # @abstract,
 *     # @interface
 *
 * This function will return two `parser::Send` nodes:
 *
 *
 * 1. `self.abstract!()`
 * 2. `self.interface!()`
 *
 * It doesn't insert them into the body of the class/module/etc.
 */
parser::NodeVec extractHelpers(core::MutableContext ctx, vector<Comment> annotations) {
    parser::NodeVec helpers;

    for (auto &annotation : annotations) {
        if (annotation.string == "abstract") {
            auto send = parser::MK::Send0(annotation.typeLoc, parser::MK::Self(annotation.typeLoc),
                                          core::Names::declareAbstract(), annotation.typeLoc);
            helpers.emplace_back(move(send));
        } else if (annotation.string == "interface") {
            auto send = parser::MK::Send0(annotation.typeLoc, parser::MK::Self(annotation.typeLoc),
                                          core::Names::declareInterface(), annotation.typeLoc);
            helpers.emplace_back(move(send));
        } else if (annotation.string == "final") {
            auto send = parser::MK::Send0(annotation.typeLoc, parser::MK::Self(annotation.typeLoc),
                                          core::Names::declareFinal(), annotation.typeLoc);
            helpers.emplace_back(move(send));
        } else if (annotation.string == "sealed") {
            auto send = parser::MK::Send0(annotation.typeLoc, parser::MK::Self(annotation.typeLoc),
                                          core::Names::declareSealed(), annotation.typeLoc);
            helpers.emplace_back(move(send));
        } else if (absl::StartsWith(annotation.string, "requires_ancestor:")) {
            if (auto type = extractHelperArgument(ctx, annotation, 18)) {
                auto body = make_unique<parser::Begin>(annotation.typeLoc, NodeVec1(move(type)));
                auto send = parser::MK::Send0(annotation.typeLoc, parser::MK::Self(annotation.typeLoc),
                                              core::Names::requiresAncestor(), annotation.typeLoc);
                auto block = make_unique<parser::Block>(annotation.typeLoc, move(send), nullptr, move(body));
                helpers.emplace_back(move(block));
            }
        }
    }

    return helpers;
}

/**
 * Wraps the body in a `parser::Begin` if it isn't already.
 *
 * This is useful for cases where we want to insert helpers into the body of a class/module/etc.
 */
unique_ptr<parser::Node> maybeWrapBody(unique_ptr<parser::Node> &owner, unique_ptr<parser::Node> body) {
    if (body == nullptr) {
        return make_unique<parser::Begin>(owner->loc, parser::NodeVec());
    } else if (parser::isa_node<parser::Begin>(body.get())) {
        return body;
    } else {
        return make_unique<parser::Begin>(body->loc, NodeVec1(move(body)));
    }
}

/**
 * Returns true if the body contains an `extend T::Helpers` call already.
 */
bool containsExtendTHelper(parser::Begin *body) {
    for (auto &stmt : body->stmts) {
        auto send = parser::cast_node<parser::Send>(stmt.get());
        if (send == nullptr) {
            continue;
        }

        if (send->method != core::Names::extend()) {
            continue;
        }

        if (send->receiver != nullptr && !parser::isa_node<parser::Self>(send->receiver.get())) {
            continue;
        }

        if (send->args.size() != 1) {
            continue;
        }

        auto arg = parser::cast_node<parser::Const>(send->args[0].get());
        if (arg == nullptr) {
            continue;
        }

        if (arg->name != core::Names::Constants::Helpers() || !parser::MK::isT(arg->scope)) {
            continue;
        }

        return true;
    }

    return false;
}

/**
 * Inserts an `extend T::Helpers` call into the body if it doesn't already exist.
 */
void maybeInsertExtendTHelpers(unique_ptr<parser::Node> *body) {
    auto begin = parser::cast_node<parser::Begin>(body->get());
    ENFORCE(begin != nullptr);

    if (containsExtendTHelper(begin)) {
        return;
    }

    auto send = parser::MK::Send1(begin->loc, parser::MK::Self(begin->loc), core::Names::extend(), begin->loc,
                                  parser::MK::T_Helpers(begin->loc));

    begin->stmts.emplace_back(move(send));
}

/**
 * Inserts the helpers into the body.
 */
void insertHelpers(unique_ptr<parser::Node> *body, parser::NodeVec helpers) {
    auto begin = parser::cast_node<parser::Begin>(body->get());
    ENFORCE(begin != nullptr);

    for (auto &helper : helpers) {
        begin->stmts.emplace_back(move(helper));
    }
}

} // namespace

void SigsRewriter::insertTypeParams(parser::Node *node, unique_ptr<parser::Node> *body) {
    ENFORCE(parser::isa_node<parser::Class>(node) || parser::isa_node<parser::Module>(node) ||
                parser::isa_node<parser::SClass>(node),
            "Unimplemented node type: {}", node->nodeName());

    auto comments = commentsForNode(node);
    if (comments.signatures.empty()) {
        return;
    }

    if (comments.signatures.size() > 1) {
        if (auto e = ctx.beginIndexerError(comments.signatures[0].commentLoc(),
                                           core::errors::Rewriter::RBSMultipleGenericSignatures)) {
            e.setHeader("Generic classes and modules can only have one RBS generic signature");
            return;
        }
    }

    auto signature = comments.signatures[0];
    auto typeParamsTranslator = SignatureTranslator(ctx);
    auto typeParams = typeParamsTranslator.translateTypeParams(signature);

    if (typeParams.empty()) {
        return;
    }

    auto begin = parser::cast_node<parser::Begin>(body->get());
    ENFORCE(begin != nullptr);

    for (auto &typeParam : typeParams) {
        begin->stmts.emplace_back(move(typeParam));
    }
}

Comments SigsRewriter::commentsForNode(parser::Node *node) {
    auto comments = Comments{};
    enum class SignatureState { None, Started, Multiline };
    auto state = SignatureState::None;

    if (auto commentsNodesEntry = commentsByNode.find(node); commentsNodesEntry != commentsByNode.end()) {
        auto commentsNodes = commentsNodesEntry->second;
        auto declaration_comments = RBSDeclaration::CommentsVector();

        for (auto &commentNode : commentsNodes) {
            // If the comment starts with `# @`, it's an annotation
            if (absl::StartsWith(commentNode.string, "# @")) {
                auto comment = Comment{
                    .commentLoc = commentNode.loc,
                    .typeLoc = core::LocOffsets{commentNode.loc.beginPos() + 3, commentNode.loc.endPos()},
                    .string = commentNode.string.substr(3),
                };

                comments.annotations.emplace_back(move(comment));
                continue;
            }

            // If the comment starts with `#:`, it's a signature
            if (absl::StartsWith(commentNode.string, "#:")) {
                if (state == SignatureState::Multiline) {
                    if (auto e =
                            ctx.beginIndexerError(commentNode.loc, core::errors::Rewriter::RBSMultilineMisformatted)) {
                        e.setHeader("Signature start (\"#:\") cannot appear after a multiline signature (\"#|\")");
                        return comments;
                    }
                }
                state = SignatureState::Started;
                auto comment = Comment{
                    .commentLoc = commentNode.loc,
                    .typeLoc = core::LocOffsets{commentNode.loc.beginPos() + 2, commentNode.loc.endPos()},
                    .string = commentNode.string.substr(2),
                };

                if (declaration_comments.empty()) {
                    declaration_comments.emplace_back(move(comment));
                } else {
                    // We already have a declaration comment, create a separate RBSDeclaration for better errors
                    // down the line
                    comments.signatures.emplace_back(
                        RBSDeclaration{move(declaration_comments)}); // Save current declaration
                    declaration_comments.clear();
                    declaration_comments.emplace_back(move(comment));
                }
                continue;
            }

            // If the comment starts with `#|`, it's a multiline signature
            if (absl::StartsWith(commentNode.string, "#|")) {
                if (state == SignatureState::None) {
                    if (auto e =
                            ctx.beginIndexerError(commentNode.loc, core::errors::Rewriter::RBSMultilineMisformatted)) {
                        e.setHeader("Multiline signature (\"#|\") must be preceded by a signature start (\"#:\")");
                        return comments;
                    }
                }
                state = SignatureState::Multiline;
                auto comment = Comment{
                    .commentLoc = commentNode.loc,
                    .typeLoc = core::LocOffsets{commentNode.loc.beginPos() + 2, commentNode.loc.endPos()},
                    .string = commentNode.string.substr(2),
                };

                declaration_comments.emplace_back(move(comment));
                continue;
            }
        }
        if (!declaration_comments.empty()) {
            auto rbsDeclaration = RBSDeclaration{move(declaration_comments)};
            comments.signatures.emplace_back(move(rbsDeclaration));
        }
    }

    return comments;
}

unique_ptr<parser::NodeVec> SigsRewriter::signaturesForNode(parser::Node *node) {
    auto comments = commentsForNode(node);

    if (comments.signatures.empty()) {
        return nullptr;
    }

    auto signatures = make_unique<parser::NodeVec>();
    signatures->reserve(comments.signatures.size());

    auto signatureTranslator = rbs::SignatureTranslator(ctx);

    for (auto &declaration : comments.signatures) {
        if (parser::isa_node<parser::DefMethod>(node) || parser::isa_node<parser::DefS>(node)) {
            auto sig = signatureTranslator.translateMethodSignature(node, declaration, comments.annotations);

            signatures->emplace_back(move(sig));
        } else if (auto send = parser::cast_node<parser::Send>(node)) {
            if (parser::MK::isMethodDefModifierSend(send)) {
                auto sig = signatureTranslator.translateMethodSignature(send->args[0].get(), declaration,
                                                                        comments.annotations);
                signatures->emplace_back(move(sig));
            } else if (parser::MK::isAttrAccessorSend(send)) {
                auto sig = signatureTranslator.translateAttrSignature(send, declaration, comments.annotations);
                signatures->emplace_back(move(sig));
            } else {
                Exception::raise("Unimplemented node type: {}", node->nodeName());
            }
        } else {
            Exception::raise("Unimplemented node type: {}", node->nodeName());
        }
    }

    return signatures;
}

/**
 * Replace the synthetic node with a `T.type_alias` call.
 */
unique_ptr<parser::Node> SigsRewriter::replaceSyntheticTypeAlias(unique_ptr<parser::Node> node) {
    auto comments = commentsForNode(node.get());

    if (comments.signatures.empty()) {
        // This should never happen
        Exception::raise("No inline comment found for synthetic type alias");
    }

    if (comments.signatures.size() > 1) {
        // This should never happen
        Exception::raise("Multiple signatures found for synthetic type alias");
    }

    auto aliasDeclaration = comments.signatures[0];
    auto typeBeginLoc = (uint32_t)aliasDeclaration.string.find("=");

    auto typeDeclaration = RBSDeclaration{{Comment{
        .commentLoc = aliasDeclaration.commentLoc(),
        .typeLoc = core::LocOffsets{aliasDeclaration.fullTypeLoc().beginPos() + typeBeginLoc + 1,
                                    aliasDeclaration.fullTypeLoc().endPos()},
        .string = aliasDeclaration.string.substr(typeBeginLoc + 1),
    }}};

    auto type = SignatureTranslator(ctx).translateType(typeDeclaration);

    if (type == nullptr) {
        type = parser::MK::TUntyped(node->loc);
    }

    return parser::MK::TTypeAlias(type->loc, move(type));
}

parser::NodeVec SigsRewriter::rewriteNodes(parser::NodeVec nodes) {
    parser::NodeVec result;
    result.reserve(nodes.size());

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
            if (auto signatures = signaturesForNode(target)) {
                for (auto &declaration : *signatures) {
                    begin->stmts.emplace_back(move(declaration));
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
        if (auto signatures = signaturesForNode(target)) {
            auto stmts = parser::NodeVec();
            stmts.reserve(signatures->size() + 1);

            for (auto &declaration : *signatures) {
                stmts.emplace_back(move(declaration));
            }

            auto loc = node->loc; // Grab the loc before moving the node out.
            stmts.emplace_back(move(node));

            return make_unique<parser::Begin>(loc, move(stmts));
        }
    }

    return rewriteNode(move(node));
}

unique_ptr<parser::Node> SigsRewriter::rewriteClass(unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    auto comments = commentsForNode(node.get());
    auto helpers = extractHelpers(ctx, comments.annotations);

    if (helpers.empty() && comments.signatures.empty()) {
        return node;
    }

    typecase(
        node.get(),
        [&](parser::Class *klass) {
            klass->body = maybeWrapBody(node, move(klass->body));
            if (!helpers.empty()) {
                maybeInsertExtendTHelpers(&klass->body);
                insertHelpers(&klass->body, move(helpers));
            }
            insertTypeParams(klass, &klass->body);
        },
        [&](parser::Module *module) {
            module->body = maybeWrapBody(node, move(module->body));
            if (!helpers.empty()) {
                maybeInsertExtendTHelpers(&module->body);
                insertHelpers(&module->body, move(helpers));
            }
            insertTypeParams(module, &module->body);
        },
        [&](parser::SClass *sclass) {
            sclass->body = maybeWrapBody(node, move(sclass->body));
            if (!helpers.empty()) {
                maybeInsertExtendTHelpers(&sclass->body);
                insertHelpers(&sclass->body, move(helpers));
            }
            insertTypeParams(sclass, &sclass->body);
        },
        [&](parser::Node *other) { Exception::raise("Unimplemented node type: {}", other->nodeName()); });

    return node;
}

parser::NodeVec SigsRewriter::synthesizeDataDefineMembers(parser::Send *send,
                                                          vector<CommentMap::DataDefineMember> &members) {
    auto loc = send->loc;
    auto signatureTranslator = rbs::SignatureTranslator(ctx);
    parser::NodeVec synthesized;

    // Build sig params for initialize: {member_name: Type, ...}
    // The sig + bare-super initialize is what Data.cc reads to produce typed readers.
    parser::NodeVec sigParams;

    for (auto &member : members) {
        unique_ptr<parser::Node> typeNode;
        auto memberLoc = member.nameLoc;

        if (member.typeComment) {
            auto &comment = *member.typeComment;
            // Strip the "#:" prefix, matching the pattern in commentsForNode().
            // The RBS parser handles any remaining leading whitespace.
            auto typeComment = Comment{
                .commentLoc = comment.loc,
                .typeLoc = core::LocOffsets{comment.loc.beginPos() + 2, comment.loc.endPos()},
                .string = comment.string.substr(2),
            };

            auto declaration = RBSDeclaration{vector<Comment>{typeComment}};
            typeNode = signatureTranslator.translateType(declaration);
        }

        if (typeNode == nullptr) {
            typeNode = parser::MK::TUntyped(memberLoc);
        }

        // Build param for initialize sig: {member_name: Type}
        sigParams.emplace_back(
            make_unique<parser::Pair>(memberLoc, parser::MK::Symbol(memberLoc, member.name), move(typeNode)));
    }

    // Synthesize initialize sig: sig {params(x: Type, y: Type).void}
    {
        auto sigBuilder = parser::MK::Self(loc);
        if (!sigParams.empty()) {
            auto hash = parser::MK::Hash(loc, true, move(sigParams));
            auto args = NodeVec1(move(hash));
            sigBuilder = parser::MK::Send(loc, move(sigBuilder), core::Names::params(), loc, move(args));
        }
        sigBuilder = parser::MK::Send0(loc, move(sigBuilder), core::Names::void_(), loc);

        auto sig = parser::MK::Send0(loc, parser::MK::T_Sig_WithoutRuntime(loc), core::Names::sig(), loc);
        auto sigBlock = make_unique<parser::Block>(loc, move(sig), nullptr, move(sigBuilder));
        synthesized.emplace_back(move(sigBlock));
    }

    // Synthesize initialize method: def initialize(member:, ...) = super
    // This MUST be bare super (ZSuper) for Data.cc's canCreateTypedAccessors() to
    // propagate types to member readers.
    {
        parser::NodeVec kwArgs;
        for (auto &member : members) {
            kwArgs.emplace_back(make_unique<parser::Kwarg>(member.nameLoc, member.name));
        }
        auto params = make_unique<parser::Params>(loc, move(kwArgs));
        auto body = make_unique<parser::ZSuper>(loc);
        auto def =
            make_unique<parser::DefMethod>(loc, loc, core::Names::initialize(), move(params), move(body));
        synthesized.emplace_back(move(def));
    }

    return synthesized;
}

// Check if the block body already contains a `def initialize` method.
// When the user provides an explicit initialize, we don't synthesize another one.
bool blockHasInitialize(parser::Block *block) {
    if (block == nullptr || block->body == nullptr) {
        return false;
    }

    auto checkNode = [](parser::Node *node) -> bool {
        auto *def = parser::cast_node<parser::DefMethod>(node);
        return def != nullptr && def->name == core::Names::initialize();
    };

    if (checkNode(block->body.get())) {
        return true;
    }

    if (auto *begin = parser::cast_node<parser::Begin>(block->body.get())) {
        for (auto &stmt : begin->stmts) {
            if (checkNode(stmt.get())) {
                return true;
            }
        }
    }

    return false;
}

// Try to rewrite a Data.define node by synthesizing a typed initialize.
// Returns nullptr if the node is not a Data.define with typed members,
// in which case the caller should proceed with normal rewriting.
unique_ptr<parser::Node> SigsRewriter::maybeRewriteDataDefine(unique_ptr<parser::Node> &node) {
    // Non-owning pointers into `node`. `send` and `block` borrow from the
    // unique_ptr and are only used to look up the dataDefineMembersByNode map
    // and to append synthesized nodes to the block body. Ownership of the tree
    // stays with `node` throughout.
    parser::Send *send = nullptr;
    parser::Block *block = nullptr;

    if (auto *b = parser::cast_node<parser::Block>(node.get())) {
        send = parser::cast_node<parser::Send>(b->send.get());
        block = b;
    } else {
        send = parser::cast_node<parser::Send>(node.get());
    }

    if (send == nullptr) {
        return nullptr;
    }

    auto it = dataDefineMembersByNode.find(send);
    if (it == dataDefineMembersByNode.end()) {
        return nullptr;
    }

    // If the block already contains an explicit `def initialize`, the user's
    // initialize takes precedence over inline type comments. Data.cc will
    // extract types from the user's sig if it has one.
    if (block != nullptr && blockHasInitialize(block)) {
        block->body = rewriteBody(move(block->body));
        return move(node);
    }

    auto &members = it->second;
    auto synthesized = synthesizeDataDefineMembers(send, members);

    if (synthesized.empty()) {
        return move(node);
    }

    if (block != nullptr) {
        // Append synthesized methods to the existing block body
        if (block->body == nullptr) {
            block->body = make_unique<parser::Begin>(block->loc, move(synthesized));
        } else if (auto *begin = parser::cast_node<parser::Begin>(block->body.get())) {
            for (auto &stmt : synthesized) {
                begin->stmts.emplace_back(move(stmt));
            }
        } else {
            // Single-node body — wrap in Begin with existing + synthesized
            parser::NodeVec stmts;
            stmts.emplace_back(move(block->body));
            for (auto &stmt : synthesized) {
                stmts.emplace_back(move(stmt));
            }
            block->body = make_unique<parser::Begin>(block->loc, move(stmts));
        }
        // Continue with normal rewriting of the block body
        block->body = rewriteBody(move(block->body));
        return move(node);
    } else {
        // No block — wrap the Send in a Block with the synthesized body.
        // Run rewriteBody for symmetry with the block case (e.g., if the synthesized
        // body contained RBS comments, though currently it doesn't).
        auto loc = send->loc;
        unique_ptr<parser::Node> body = make_unique<parser::Begin>(loc, move(synthesized));
        body = rewriteBody(move(body));
        return make_unique<parser::Block>(loc, move(node), nullptr, move(body));
    }
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
            if (auto rewritten = maybeRewriteDataDefine(node)) {
                result = move(rewritten);
                return;
            }
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
            result = rewriteClass(move(node));
        },
        [&](parser::Class *klass) {
            klass->body = rewriteBody(move(klass->body));
            result = rewriteClass(move(node));
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
            result = rewriteClass(move(node));
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
        [&](parser::Send *send) {
            if (auto rewritten = maybeRewriteDataDefine(node)) {
                result = move(rewritten);
                return;
            }
            send->args = rewriteNodes(move(send->args));
            result = move(node);
        },
        [&](parser::RBSPlaceholder *placeholder) {
            if (placeholder->kind == core::Names::Constants::RBSTypeAlias()) {
                result = replaceSyntheticTypeAlias(move(node));
            } else {
                result = move(node);
            }
        },
        [&](parser::Node *other) { result = move(node); });

    return result;
}

unique_ptr<parser::Node> SigsRewriter::run(unique_ptr<parser::Node> node) {
    // If there are no signature comments or Data.define member types to process,
    // we can skip the entire tree walk.
    if (commentsByNode.empty() && dataDefineMembersByNode.empty()) {
        return node;
    }
    return rewriteBody(move(node));
}

} // namespace sorbet::rbs
