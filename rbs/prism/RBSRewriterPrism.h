#ifndef SORBET_RBS_RBS_REWRITER_PRISM_H
#define SORBET_RBS_RBS_REWRITER_PRISM_H

#include "core/Context.h"
#include "parser/prism/Parser.h"

namespace sorbet::rbs {

void runPrismRBSRewrite(core::GlobalState &gs, core::FileRef file, core::MutableContext &ctx,
                        parser::Prism::ParseResult &parseResult);

} // namespace sorbet::rbs

#endif // SORBET_RBS_RBS_REWRITER_PRISM_H
