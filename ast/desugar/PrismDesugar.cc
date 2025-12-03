#include <algorithm>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_replace.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "ast/desugar/DuplicateHashKeyCheck.h"
#include "ast/desugar/PrismDesugar.h"
#include "ast/verifier/verifier.h"
#include "common/common.h"
#include "common/strings/formatting.h"
#include "core/Names.h"
#include "core/errors/desugar.h"
#include "core/errors/internal.h"

// Forward declaration to avoid including parser/prism/Parser.h which has additional dependencies
namespace sorbet::parser::Prism {
struct PrismFallback {};
} // namespace sorbet::parser::Prism

// The Prism Desugarer starts out as a near-exact copy of the legacy Desugarer. Over time, we migrate more and more of
// its logic out and into `parser/prism/Translator.cc`. Any changes made to the "upstream" Desugarer also need to be
// refected here, and in the Translator.
//
// One key difference of the Prism desugarer is that it calls `NodeWithExpr::cast_node` and `NodeWithExpr::isa_node`
// instead of the usual `parser::cast_node` and `parser::isa_node` functions. This adds extra overhead in the Prism
// case, but remains zero-cost in the case of the legacy parser and the regular `Desugar.cc`.
namespace sorbet::ast::prismDesugar {

using namespace std;
using sorbet::ast::desugar::DuplicateHashKeyCheck;

namespace {

// Translate a tree to an expression. NOTE: this should only be called from `node2TreeImpl`.
ExpressionPtr node2TreeImplBody(parser::Node *what) {
    ENFORCE(what != nullptr);

    if (auto *nodeWithExpr = parser::NodeWithExpr::cast_node<parser::NodeWithExpr>(what)) {
        if (parser::NodeWithExpr::isa_node<parser::Splat>(nodeWithExpr->wrappedNode.get())) {
            // Special case for Splats in method calls where we want zero-length locations
            // The `parser::Send` case makes a fake parser::Array with locZeroLen to hide callWithSplat
            // methods from hover.`
            auto splat = parser::NodeWithExpr::cast_node<parser::Splat>(nodeWithExpr->wrappedNode.get());

            if (!splat->var->hasDesugaredExpr()) {
                throw parser::Prism::PrismFallback{};
            }

            return MK::Splat(what->loc, splat->var->takeDesugaredExpr());
        }
    }

    ENFORCE(what->hasDesugaredExpr(), "Node has no desugared expression");
    auto expr = what->takeDesugaredExpr();
    ENFORCE(expr != nullptr, "Node has null desugared expr");
    return expr;
}

// Translate trees by calling `node2TreeBody`, and manually reset the unique_ptr argument when it's done.
ExpressionPtr node2TreeImpl(unique_ptr<parser::Node> &what) {
    auto res = node2TreeImplBody(what.get());
    what.reset();
    return res;
}

ExpressionPtr liftTopLevel(core::LocOffsets loc, ExpressionPtr what) {
    ClassDef::RHS_store rhs;
    ClassDef::ANCESTORS_store ancestors;
    ancestors.emplace_back(MK::Constant(loc, core::Symbols::todo()));
    auto insSeq = cast_tree<InsSeq>(what);
    if (insSeq) {
        rhs.reserve(insSeq->stats.size() + 1);
        for (auto &stat : insSeq->stats) {
            rhs.emplace_back(move(stat));
        }
        rhs.emplace_back(move(insSeq->expr));
    } else {
        rhs.emplace_back(move(what));
    }
    return make_expression<ClassDef>(loc, loc, core::Symbols::root(), MK::EmptyTree(), move(ancestors), move(rhs),
                                     ClassDef::Kind::Class);
}
} // namespace

ExpressionPtr node2Tree(core::MutableContext ctx, unique_ptr<parser::Node> what, bool preserveConcreteSyntax) {
    try {
        auto liftedClassDefLoc = what->loc;
        auto result = node2TreeImpl(what);
        if (result.loc().exists()) {
            // If the desugared expression has a different loc, we want to use that. This can happen
            // because (:block (:send)) desugars to (:send (:block)), but the (:block) node just has
            // the loc of the `do ... end`, while the (:send) has the whole loc
            //
            // But if we desugared to EmptyTree (either intentionally or because there was an
            // unsupported node type), we want to use the loc of the original node.
            liftedClassDefLoc = result.loc();
        }
        result = liftTopLevel(liftedClassDefLoc, move(result));
        auto verifiedResult = Verifier::run(ctx, move(result));
        return verifiedResult;
    } catch (SorbetException &) {
        throw;
    }
}
} // namespace sorbet::ast::prismDesugar
