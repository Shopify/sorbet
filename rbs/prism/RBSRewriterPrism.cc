#include "rbs/prism/RBSRewriterPrism.h"

#include "common/timers/Timer.h"
#include "rbs/prism/AssertionsRewriterPrism.h"
#include "rbs/prism/CommentsAssociatorPrism.h"
#include "rbs/prism/SigsRewriterPrism.h"

using namespace std;

namespace sorbet::rbs {

void runPrismRBSRewrite(core::GlobalState &gs, core::FileRef file, core::MutableContext &ctx,
                        parser::Prism::ParseResult &parseResult) {
    Timer timeit(gs.tracer(), "runPrismRBSRewrite", {{"file", string(file.data(gs).path())}});

    auto &parser = parseResult.getParser();
    auto *node = parseResult.getRawNodePointer();

    auto associator = CommentsAssociatorPrism(ctx, parser, parseResult.getCommentLocations());
    auto commentMap = associator.run(node);

    auto sigsRewriter = SigsRewriterPrism(ctx, parser, commentMap.signaturesForNode, parseResult);
    node = sigsRewriter.run(node);

    auto assertionsRewriter = AssertionsRewriterPrism(ctx, parser, commentMap.assertionsForNode, parseResult);
    node = assertionsRewriter.run(node);

    parseResult.replaceRootNode(node);
}

} // namespace sorbet::rbs
