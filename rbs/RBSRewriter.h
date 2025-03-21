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

struct InlineComment {
    enum class Kind {
        LET,
        CAST,
        MUST,
    };

    rbs::Comment comment;
    Kind kind;
};

class RBSRewriter {
public:
    RBSRewriter(core::MutableContext ctx) : ctx(ctx), lastSignature(nullptr){};
    std::unique_ptr<parser::Node> rewriteNode(std::unique_ptr<parser::Node> tree);

private:
    core::MutableContext ctx;
    parser::Node *lastSignature;

    std::unique_ptr<parser::Node> rewriteBegin(std::unique_ptr<parser::Node> tree);
    std::unique_ptr<parser::Node> rewriteBody(std::unique_ptr<parser::Node> tree);
    parser::NodeVec rewriteNodes(parser::NodeVec nodes);
    std::unique_ptr<parser::Node> getRBSAssertionType(std::unique_ptr<parser::Node> &node, core::LocOffsets fromLoc);
    std::optional<rbs::Comment> findRBSTrailingComment(std::unique_ptr<parser::Node> &node, core::LocOffsets fromLoc);
    bool isHeredoc(core::LocOffsets assignLoc, const std::unique_ptr<parser::Node> &node);
    bool hasHeredocMarker(const uint32_t fromPos, const uint32_t toPos);
    std::unique_ptr<parser::NodeVec> getRBSSignatures(std::unique_ptr<parser::Node> &node);
    Comments findRBSSignatureComments(std::string_view sourceCode, core::LocOffsets loc);
    void maybeSaveSignature(parser::Block *block);
    std::vector<std::pair<core::LocOffsets, core::NameRef>> lastTypeParams();
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_REWRITER_H
