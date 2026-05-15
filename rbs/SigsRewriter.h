#ifndef SORBET_RBS_SIGS_REWRITER_H
#define SORBET_RBS_SIGS_REWRITER_H

#include "parser/parser.h"
#include "rbs/CommentsAssociator.h"
#include "rbs/rbs_common.h"
#include <memory>

namespace sorbet::rbs {

/**
 * A collection of annotations and signatures comments found on a method definition.
 */
struct Comments {
    /**
     * RBS annotation comments found on a method definition.
     *
     * Annotations are formatted as `@some_annotation`.
     */
    std::vector<Comment> annotations;

    /**
     * RBS signature comments found on a method definition.
     *
     * Signatures are formatted as `#: () -> void`.
     */
    std::vector<RBSDeclaration> signatures;
};

class SigsRewriter {
public:
    SigsRewriter(core::MutableContext ctx, std::map<parser::Node *, std::vector<rbs::CommentNode>> &commentsByNode,
                 std::map<parser::Node *, std::vector<rbs::CommentMap::DataDefineMember>> &dataDefineMembersByNode)
        : ctx(ctx), commentsByNode(commentsByNode), dataDefineMembersByNode(dataDefineMembersByNode){};
    std::unique_ptr<parser::Node> run(std::unique_ptr<parser::Node> tree);

private:
    core::MutableContext ctx;
    // Non-owning references to CommentMap fields. The CommentMap is owned by the
    // pipeline caller (runRBSRewrite) and outlives the SigsRewriter.
    std::map<parser::Node *, std::vector<rbs::CommentNode>> &commentsByNode;
    std::map<parser::Node *, std::vector<rbs::CommentMap::DataDefineMember>> &dataDefineMembersByNode;

    std::unique_ptr<parser::Node> rewriteBegin(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteBody(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteNode(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteClass(std::unique_ptr<parser::Node> tree);
    parser::NodeVec rewriteNodes(parser::NodeVec nodes);
    std::unique_ptr<parser::NodeVec> signaturesForNode(parser::Node *node);
    Comments commentsForNode(parser::Node *node);
    void insertTypeParams(parser::Node *node, std::unique_ptr<parser::Node> *body);
    std::unique_ptr<parser::Node> replaceSyntheticTypeAlias(std::unique_ptr<parser::Node> node);
    std::unique_ptr<parser::Node> maybeRewriteDataDefine(std::unique_ptr<parser::Node> &node);
    parser::NodeVec synthesizeDataDefineMembers(parser::Send *send,
                                                std::vector<rbs::CommentMap::DataDefineMember> &members);
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_SIGS_REWRITER_H
