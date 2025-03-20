#ifndef SORBET_RBS_REWRITER_H
#define SORBET_RBS_REWRITER_H

#include "parser/parser.h"
#include <memory>

namespace sorbet::rbs {

std::unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, std::unique_ptr<parser::Node> tree);

} // namespace sorbet::rbs

#endif // SORBET_RBS_REWRITER_H
