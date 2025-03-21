#ifndef SORBET_RBS_REWRITER_H
#define SORBET_RBS_REWRITER_H

#include "rbs/SigsRewriter.h"
#include "rbs/rbs_common.h"
#include <memory>

namespace sorbet::rbs {

class RBSRewriter {
public:
    RBSRewriter(core::MutableContext ctx) : ctx(ctx), lastSignature(nullptr){};
    std::unique_ptr<parser::Node> run(std::unique_ptr<parser::Node> tree);

private:
    core::MutableContext ctx;
    parser::Node *lastSignature;

    std::unique_ptr<parser::Node> rewriteNode(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteBegin(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteBody(std::unique_ptr<parser::Node> tree);
    parser::NodeVec rewriteNodes(parser::NodeVec nodes);
    std::optional<std::pair<std::unique_ptr<parser::Node>, InlineComment::Kind>>
    getRBSAssertionType(std::unique_ptr<parser::Node> &node, core::LocOffsets fromLoc);
    std::optional<rbs::InlineComment> findRBSTrailingCommentFromPos(uint32_t fromPos);
    std::optional<rbs::InlineComment> findRBSTrailingComment(std::unique_ptr<parser::Node> &node,
                                                             core::LocOffsets fromLoc);
    bool isHeredoc(core::LocOffsets assignLoc, const std::unique_ptr<parser::Node> &node);
    bool hasHeredocMarker(const uint32_t fromPos, const uint32_t toPos);
    void maybeSaveSignature(parser::Block *block);
    std::vector<std::pair<core::LocOffsets, core::NameRef>> lastTypeParams();
    std::unique_ptr<parser::Node> maybeInsertRBSCast(std::unique_ptr<parser::Node> node);
    void insertSignatures(parser::NodeVec &stmts, parser::NodeVec &signatures);
    std::unique_ptr<parser::Node> wrapInBegin(std::unique_ptr<parser::Node> node, parser::NodeVec &signatures);
    std::unique_ptr<parser::Node> insertRBSCast(std::unique_ptr<parser::Node> node, std::unique_ptr<parser::Node> type,
                                                InlineComment::Kind kind);
    bool isVisibilitySend(parser::Send *send);
    bool isAttrAccessorSend(parser::Send *send);
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_REWRITER_H
