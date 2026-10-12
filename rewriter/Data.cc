#include "rewriter/Data.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/Context.h"
#include "core/Names.h"
#include "core/core.h"
#include "core/errors/rewriter.h"
#include "rewriter/util/Util.h"

using namespace std;

namespace sorbet::rewriter {

namespace {

bool isMissingInitialize(const core::GlobalState &gs, const ast::Send *send) {
    if (!send->hasBlock()) {
        return true;
    }

    auto block = send->block();

    if (auto insSeq = ast::cast_tree<ast::InsSeq>(block->body)) {
        auto methodDef = ast::cast_tree<ast::MethodDef>(insSeq->expr);

        if (methodDef && methodDef->name == core::Names::initialize()) {
            return false;
        }

        for (auto &&stat : insSeq->stats) {
            methodDef = ast::cast_tree<ast::MethodDef>(stat);

            if (methodDef && methodDef->name == core::Names::initialize()) {
                return false;
            }
        }
    }

    return true;
}

// Find the params Send inside a sig block body. Navigates through the
// method chain like: self.params(...).void() or self.params(...).returns(...)
//
// Returns a non-owning pointer into the sig's AST. The returned pointer borrows
// from the ExpressionPtr tree rooted at `sigBlockBody`; it remains valid as long
// as that tree is alive (i.e., until the block body is move()'d later in Data::run).
const ast::Send *findSigParamsCall(const ast::ExpressionPtr &sigBlockBody) {
    auto current = ast::cast_tree<ast::Send>(sigBlockBody);
    while (current) {
        if (current->fun == core::Names::params()) {
            return current;
        }
        current = ast::cast_tree<ast::Send>(current->recv);
    }
    return nullptr;
}

// Check whether a MethodDef body is exactly a bare `super` call (i.e.,
// `def initialize(...) = super` which desugars to `send(untypedSuper, [ZSuperArgs])`).
// Only when the body is bare super can we reliably propagate sig types to readers,
// because the values are forwarded to Data's internal storage unchanged.
//
// `methodDef` is a non-owning pointer borrowed from the block body's InsSeq.
bool isBareSuper(const ast::MethodDef *methodDef) {
    auto send = ast::cast_tree<ast::Send>(methodDef->rhs);
    if (!send || send->fun != core::Names::untypedSuper()) {
        return false;
    }
    return send->numPosArgs() == 1 && ast::isa_tree<ast::ZSuperArgs>(send->getPosArg(0));
}

// Find the sig Send that immediately precedes a def initialize in an InsSeq,
// but only if the initialize body is bare `super`. Returns nullptr otherwise.
//
// Returns a non-owning pointer into the InsSeq's statement list. The pointer
// borrows from the Data.define block body and remains valid until that body is
// move()'d into the synthesized class definition later in Data::run.
//
// We intentionally restrict typed readers to bare-super initializers because
// if the user transforms values (e.g. `super(x: x.to_i)`), the reader types
// would not match the sig's parameter types. This conservative approach follows
// the principle that "Sorbet doesn't do anything" unless precise constraints
// are met, keeping our options open for smarter analysis in the future.
const ast::Send *findSigBeforeInitialize(const ast::InsSeq &insSeq) {
    for (size_t i = 0; i < insSeq.stats.size(); i++) {
        auto candidateSig = ast::cast_tree<ast::Send>(insSeq.stats[i]);
        if (!candidateSig || candidateSig->fun != core::Names::sig()) {
            continue;
        }

        // Check if the next statement is def initialize with bare super
        const ast::MethodDef *initDef = nullptr;
        if (i + 1 < insSeq.stats.size()) {
            initDef = ast::cast_tree<ast::MethodDef>(insSeq.stats[i + 1]);
        }
        if (!initDef && i + 1 == insSeq.stats.size()) {
            initDef = ast::cast_tree<ast::MethodDef>(insSeq.expr);
        }

        if (initDef && initDef->name == core::Names::initialize() && isBareSuper(initDef)) {
            return candidateSig;
        }
    }
    return nullptr;
}

// Extract parameter name -> type mappings from a sig's params(...) call.
// `paramsCall` is a non-owning pointer borrowed from the sig's block body.
// Type expressions are deep-copied into `types` so the output is independent
// of the original AST's lifetime.
//
// Handles two formats:
//   - Keyword args (from RBS-created sigs via desugaring): params(x: Integer, y: String)
//     → numPosArgs=0, kwargs flattened as alternating key/value pairs
//   - Positional pairs (from rewriter-synthesized sigs): params(:x, Integer, :y, String)
//     → numPosArgs=N, all args positional
void extractTypesFromParams(const ast::Send *paramsCall,
                            UnorderedMap<core::NameRef, ast::ExpressionPtr> &types) {
    if (paramsCall->numKwArgs() > 0) {
        for (uint16_t i = 0; i < paramsCall->numKwArgs(); i++) {
            auto key = ast::cast_tree<ast::Literal>(paramsCall->getKwKey(i));
            if (key && key->isSymbol()) {
                types[key->asSymbol()] = paramsCall->getKwValue(i).deepCopy();
            }
        }
    } else if (paramsCall->numPosArgs() >= 2) {
        for (uint16_t i = 0; i + 1 < paramsCall->numPosArgs(); i += 2) {
            auto key = ast::cast_tree<ast::Literal>(paramsCall->getPosArg(i));
            if (key && key->isSymbol()) {
                types[key->asSymbol()] = paramsCall->getPosArg(i + 1).deepCopy();
            }
        }
    }
}

// Extract parameter name -> type mappings from a sig preceding def initialize
// in the block body. Returns a map from member name to deep-copied type expressions.
//
// The `send` parameter is a non-owning pointer to the Data.define Send node,
// borrowed from the Assign node in Data::run. We read from it (to find the sig
// and extract types) before the block body is move()'d into the synthesized class.
// The returned map contains deep copies of the type expressions, so it is safe to
// use after the original AST nodes have been moved.
UnorderedMap<core::NameRef, ast::ExpressionPtr> extractInitializeTypes(const ast::Send *send) {
    UnorderedMap<core::NameRef, ast::ExpressionPtr> types;

    if (!send->hasBlock()) {
        return types;
    }

    auto block = send->block();
    auto insSeq = ast::cast_tree<ast::InsSeq>(block->body);
    if (!insSeq) {
        return types;
    }

    auto sigSend = findSigBeforeInitialize(*insSeq);
    if (!sigSend || !sigSend->hasBlock()) {
        return types;
    }

    auto sigBlock = sigSend->block();
    auto paramsCall = findSigParamsCall(sigBlock->body);
    if (!paramsCall) {
        return types;
    }

    extractTypesFromParams(paramsCall, types);
    return types;
}

} // namespace

vector<ast::ExpressionPtr> Data::run(core::MutableContext ctx, ast::Assign *asgn) {
    vector<ast::ExpressionPtr> empty;

    if (ctx.state.cacheSensitiveOptions.runningUnderAutogen) {
        return empty;
    }

    auto lhs = ast::cast_tree<ast::UnresolvedConstantLit>(asgn->lhs);
    if (lhs == nullptr) {
        return empty;
    }

    auto send = ast::cast_tree<ast::Send>(asgn->rhs);
    if (send == nullptr) {
        return empty;
    }

    if (!ASTUtil::isRootScopedSyntacticConstant(send->recv, {core::Names::Constants::Data()})) {
        return empty;
    }

    if (send->fun != core::Names::define() || send->hasKwArgs() || send->hasKwSplat()) {
        return empty;
    }

    auto loc = asgn->loc;

    ast::MethodDef::PARAMS_store newArgs;
    ast::Send::ARGS_store sigArgs;
    ast::ClassDef::RHS_store body;

    if (auto dup = ASTUtil::findDuplicateArg(ctx, send)) {
        if (auto e = ctx.beginIndexerError(dup->secondLoc, core::errors::Rewriter::InvalidStructMember)) {
            e.setHeader("Duplicate member `{}` in Data definition", dup->name.show(ctx));
            e.addErrorLine(ctx.locAt(dup->firstLoc), "First occurrence of `{}` in Data definition",
                           dup->name.show(ctx));
        }
        return empty;
    }

    // If the block contains a typed def initialize (with a sig and bare super),
    // extract the parameter types so we can create typed reader stubs.
    // This must happen BEFORE the block body is move()'d below — the extraction
    // reads from the block's AST and deep-copies the type expressions into
    // `memberTypes`, which is then safe to use after the move.
    auto memberTypes = extractInitializeTypes(send);

    for (auto &arg : send->posArgs()) {
        auto sym = ast::cast_tree<ast::Literal>(arg);
        if (!sym || !sym->isName()) {
            return empty;
        }
        core::NameRef name = sym->asName();
        auto symLoc = sym->loc;
        auto strname = name.shortName(ctx);
        if (!strname.empty() && strname.back() == '=') {
            if (auto e = ctx.beginIndexerError(symLoc, core::errors::Rewriter::InvalidStructMember)) {
                e.setHeader("Data member `{}` cannot end with an equal", strname);
            }
        }

        if (symLoc.exists() && ctx.locAt(symLoc).adjustLen(ctx, 0, 1).source(ctx) == ":") {
            symLoc = ctx.locAt(symLoc).adjust(ctx, 1, 0).offsets();
        }

        sigArgs.emplace_back(ast::MK::Symbol(symLoc, name));
        sigArgs.emplace_back(ast::MK::Constant(symLoc, core::Symbols::BasicObject()));

        auto argName = ast::MK::Local(symLoc, name);
        newArgs.emplace_back(ast::MK::OptionalParam(symLoc, move(argName), ast::MK::Nil(symLoc)));

        // If we have a typed initialize, create a typed reader with a sig.
        // Otherwise, create an untyped reader (existing behavior).
        auto it = memberTypes.find(name);
        if (it != memberTypes.end()) {
            body.emplace_back(ast::MK::Sig0(symLoc.copyWithZeroLength(), move(it->second)));
        }
        body.emplace_back(ast::MK::SyntheticMethod0(symLoc, symLoc, name, ast::MK::RaiseUnimplemented(loc)));
    }

    if (isMissingInitialize(ctx, send)) {
        body.emplace_back(ast::MK::SigVoid(loc, std::move(sigArgs)));
        body.emplace_back(ast::MK::SyntheticMethod(loc, loc, core::Names::initialize(), std::move(newArgs),
                                                   ast::MK::RaiseUnimplemented(loc)));
    }

    if (auto *block = send->block()) {
        // Steal the trees, because the run is going to remove the original send node from the tree anyway.
        if (auto insSeq = ast::cast_tree<ast::InsSeq>(block->body)) {
            for (auto &&stat : insSeq->stats) {
                body.emplace_back(move(stat));
            }
            body.emplace_back(move(insSeq->expr));
        } else {
            body.emplace_back(move(block->body));
        }

        // NOTE: the code in this block _STEALS_ trees. No _return empty_'s should go after it
    }

    ast::ClassDef::ANCESTORS_store ancestors;
    ancestors.emplace_back(ast::MK::UnresolvedConstant(loc, ast::MK::Constant(loc, core::Symbols::root()),
                                                       core::Names::Constants::Data()));

    vector<ast::ExpressionPtr> stats;
    stats.emplace_back(ast::MK::Class(loc, loc, std::move(asgn->lhs), std::move(ancestors), std::move(body)));
    return stats;
}

}; // namespace sorbet::rewriter
