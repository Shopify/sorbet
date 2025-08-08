#ifndef SORBET_RBS_ASSERTIONS_REWRITER_PRISM_H
#define SORBET_RBS_ASSERTIONS_REWRITER_PRISM_H

#include "parser/parser.h"
#include "parser/prism/Helpers.h"
#include "parser/prism/Parser.h"
#include "rbs/prism/CommentsAssociatorPrism.h"
#include "rbs/rbs_common.h"
#include <memory>

extern "C" {
#include "prism.h"
}

namespace sorbet::rbs {

struct InlineCommentPrism {
    enum class Kind {
        ABSURD,
        BIND,
        CAST,
        LET,
        MUST,
        UNSAFE,
    };

    Comment comment;
    Kind kind;
};

class AssertionsRewriterPrism {
public:
    AssertionsRewriterPrism(core::MutableContext ctx, parser::Prism::Parser &parser,
                            std::unordered_map<pm_node_t *, std::vector<CommentNodePrism>> &commentsByNode)
        : ctx(ctx), parser(parser), prism(parser), commentsByNode(&commentsByNode) {};
    pm_node_t *run(pm_node_t *node);

private:
    core::MutableContext ctx;
    parser::Prism::Parser &parser;
    parser::Prism::Factory prism;
    std::unordered_map<pm_node_t *, std::vector<CommentNodePrism>> *commentsByNode;
    std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams = {};
    std::set<std::pair<uint32_t, uint32_t>> consumedComments = {};

    void consumeComment(core::LocOffsets loc);
    bool hasConsumedComment(core::LocOffsets loc);
    std::optional<InlineCommentPrism> commentForPos(uint32_t fromPos, std::vector<char> allowedTokens);
    std::optional<InlineCommentPrism> commentForNode(const std::unique_ptr<parser::Node> &node);
    std::optional<InlineCommentPrism> commentForNode(pm_node_t *node);

    core::LocOffsets translateLocation(pm_location_t location);

    pm_node_t *rewriteBody(pm_node_t *tree);
    pm_node_t *rewriteNode(pm_node_t *tree);
    void rewriteNodes(pm_node_list_t &nodes);
    void rewriteNodesAsArray(pm_node_t *node, pm_node_list_t &nodes);

    bool saveTypeParams(pm_node_t *call);
    pm_node_t *maybeInsertCast(pm_node_t *node);
    pm_node_t *insertCast(pm_node_t *node, std::optional<std::pair<pm_node_t *, InlineCommentPrism::Kind>> pair);
    pm_node_t *replaceSyntheticBind(pm_node_t *node);

    void checkDanglingCommentWithDecl(uint32_t nodeEnd, uint32_t declEnd, std::string kind);
    void checkDanglingComment(uint32_t nodeEnd, std::string kind);
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_ASSERTIONS_REWRITER_PRISM_H
