#ifndef SORBET_RBS_REWRITER_H
#define SORBET_RBS_REWRITER_H

#include "parser/parser.h"
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
    std::vector<rbs::Comment> annotations;

    /**
     * RBS signature comments found on a method definition.
     *
     * Signatures are formatted as `#: () -> void`.
     */
    std::vector<rbs::Comment> signatures;
};

class RBSRewriter {
public:
    std::unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, std::unique_ptr<parser::Node> tree);

private:
    std::unique_ptr<parser::Node> rewriteBegin(core::MutableContext ctx, std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteBody(core::MutableContext ctx, std::unique_ptr<parser::Node> tree);
    parser::NodeVec rewriteNodes(core::MutableContext ctx, parser::NodeVec nodes);
    std::unique_ptr<parser::Node> getRBSAssertionType(core::MutableContext ctx, std::unique_ptr<parser::Node> &node,
                                                      core::LocOffsets fromLoc);
    std::optional<rbs::Comment> findRBSTrailingComment(core::MutableContext ctx, std::unique_ptr<parser::Node> &node,
                                                       core::LocOffsets fromLoc);
    bool isHeredoc(core::MutableContext ctx, core::LocOffsets assignLoc, const std::unique_ptr<parser::Node> &node);
    bool hasHeredocMarker(core::MutableContext ctx, const uint32_t fromPos, const uint32_t toPos);
    std::unique_ptr<parser::NodeVec> getRBSSignatures(core::MutableContext ctx, std::unique_ptr<parser::Node> &node);
    Comments findRBSSignatureComments(std::string_view sourceCode, core::LocOffsets loc);
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_REWRITER_H
