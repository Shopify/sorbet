#ifndef SORBET_RBS_SIGS_REWRITER_PRISM_H
#define SORBET_RBS_SIGS_REWRITER_PRISM_H

#include "parser/parser.h"
#include "parser/prism/Parser.h"
#include "rbs/prism/CommentsAssociatorPrism.h"

extern "C" {
#include "prism.h"
}

namespace sorbet::rbs {

class SigsRewriterPrism {
public:
    SigsRewriterPrism(core::MutableContext ctx, const parser::Prism::Parser &parser,
                      std::map<pm_node_t *, std::vector<CommentNodePrism>> &commentsByNode);

    pm_node_t *run(pm_node_t *node);

private:
    core::MutableContext ctx;
    [[maybe_unused]] const parser::Prism::Parser &parser;
    [[maybe_unused]] std::map<pm_node_t *, std::vector<CommentNodePrism>> *commentsByNode;
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_SIGS_REWRITER_PRISM_H
