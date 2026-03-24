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

// Find the `params(...)` Send node inside a sig block.
// Walks the sig's builder chain (e.g., `self.params(...).void`) to find the params call.
//
// Returns a non-owning pointer into the sig's AST. The returned pointer borrows from the
// ExpressionPtr pointed to by `send`, which is itself a node in the block body. Valid as
// long as the block body's ExpressionPtr tree is alive (i.e., before it is move()'d).
const ast::Send *findParams(const ast::ExpressionPtr *send) {
    if (send == nullptr) {
        return nullptr;
    }
    auto *sig = ASTUtil::castSig(*send);
    if (sig == nullptr) {
        return nullptr;
    }

    auto *block = sig->block();
    if (block == nullptr) {
        return nullptr;
    }

    auto bodyBlock = ast::cast_tree<ast::Send>(block->body);

    while (bodyBlock && bodyBlock->fun != core::Names::params()) {
        bodyBlock = ast::cast_tree<ast::Send>(bodyBlock->recv);
    }

    return bodyBlock;
}

// Information about a user-defined `initialize` found inside a Data.define block.
//
// Both pointers are non-owning borrows into the block body of the Data.define Send node.
// The block body's ExpressionPtr tree owns these nodes and is responsible for their
// lifetime. They remain valid throughout `Data::run` because we read from them (to
// extract types and check for bare `super`) before the block body is move()'d into the
// synthesized class definition.
struct InitializeInfo {
    // The initialize method definition. Used to check whether its body is bare `super`,
    // which is the only case where we can reliably propagate types to attribute readers.
    const ast::MethodDef *methodDef;

    // The `params(...)` Send node from the sig immediately preceding initialize, or
    // nullptr if there was no sig. Used to extract member types.
    const ast::Send *sigParams;
};

// Search the Data.define block body for a `def initialize` method definition.
// If found, also looks for a sig immediately preceding it.
optional<InitializeInfo> getInitialize(const ast::Send *send) {
    if (!send->hasBlock()) {
        return nullopt;
    }

    auto block = send->block();

    if (auto insSeq = ast::cast_tree<ast::InsSeq>(block->body)) {
        const ast::ExpressionPtr *prevStat = nullptr;
        for (auto &stat : insSeq->stats) {
            auto methodDef = ast::cast_tree<ast::MethodDef>(stat);

            if (methodDef && methodDef->name == core::Names::initialize()) {
                return InitializeInfo{methodDef, findParams(prevStat)};
            }

            prevStat = &stat;
        }

        // the last expression of the block is stored separately as expr
        auto methodDef = ast::cast_tree<ast::MethodDef>(insSeq->expr);
        if (methodDef && methodDef->name == core::Names::initialize()) {
            return InitializeInfo{methodDef, findParams(prevStat)};
        }
    } else if (auto methodDef = ast::cast_tree<ast::MethodDef>(block->body)) {
        if (methodDef && methodDef->name == core::Names::initialize()) {
            return InitializeInfo{methodDef, nullptr};
        }
    }

    return nullopt;
}

// Check if the initialize body is exactly bare `super` (i.e., forwards all args).
// Only when this is true can we reliably propagate sig types to attribute readers.
// When the user transforms values (e.g., `super(x: x.to_i)`), the sig types describe
// the initialize params, not necessarily what gets stored — so readers must fall back
// to T.untyped.
bool canCreateTypedAccessors(const core::GlobalState &gs, const ast::MethodDef *initialize) {
    auto send = ast::cast_tree<ast::Send>(initialize->rhs);
    if (!send)
        return false;
    if (send->fun != core::Names::untypedSuper())
        return false;
    if (send->numPosArgs() == 1 && ast::isa_tree<ast::ZSuperArgs>(send->getPosArg(0)))
        return true;

    return false;
}

// Extract the type for a specific member from the sig's params() call.
// `params` is a non-owning pointer borrowed from the sig AST (see findParams).
// Returns a deep copy of the type expression, or T.untyped if the member isn't found.
ast::ExpressionPtr getMemberType(core::MutableContext ctx, const ast::Send *params, core::NameRef name,
                                 core::LocOffsets loc) {
    if (params != nullptr) {
        for (int i = 0; i < params->numKwArgs(); i++) {
            auto lit = ast::cast_tree<ast::Literal>(params->getKwKey(i));
            if (!lit) {
                continue;
            }
            auto key = lit->asName();

            if (key.toString(ctx) == name.toString(ctx)) {
                return params->getKwValue(i).deepCopy();
            }
        }
    }

    return ast::MK::Untyped(loc);
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

    if (auto dup = ASTUtil::findDuplicateArg(ctx, send)) {
        if (auto e = ctx.beginIndexerError(dup->secondLoc, core::errors::Rewriter::InvalidStructMember)) {
            e.setHeader("Duplicate member `{}` in Data definition", dup->name.show(ctx));
            e.addErrorLine(ctx.locAt(dup->firstLoc), "First occurrence of `{}` in Data definition",
                           dup->name.show(ctx));
        }
        return empty;
    }

    // Check for a user-provided initialize with a sig in the block.
    // `reliableSigParams` is a non-owning pointer into the block body's AST tree.
    // It borrows from `send->block()->body` and remains valid until the block body
    // is move()'d into the synthesized class (the "steal" section below).
    // We only read from it (via getMemberType) before that move happens.
    auto initialize = getInitialize(send);
    bool initializeHasSig = false;
    const ast::Send *reliableSigParams = nullptr;
    if (initialize.has_value()) {
        initializeHasSig = !!initialize->sigParams;
        if (initializeHasSig && canCreateTypedAccessors(ctx, initialize->methodDef)) {
            reliableSigParams = initialize->sigParams;
        }
    }

    auto loc = asgn->loc;
    ast::Send::ARGS_store newSigArgs;
    ast::MethodDef::PARAMS_store newArgs;
    ast::Send::ARGS_store initializeSigArgs;
    ast::MethodDef::PARAMS_store initializeArgs;
    ast::ClassDef::RHS_store body;

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

        auto memberType = getMemberType(ctx, reliableSigParams, name, symLoc);

        // Generate typed reader: sig {returns(Type)} + def member_name; end
        body.emplace_back(ast::MK::Sig0(symLoc, ASTUtil::dupType(memberType)));
        body.emplace_back(ast::MK::SyntheticMethod0(symLoc, symLoc, name, ast::MK::RaiseUnimplemented(loc)));

        // Build self.new args (only when no user-provided initialize with sig)
        if (!(initialize.has_value() && initializeHasSig)) {
            newSigArgs.emplace_back(ast::MK::Symbol(symLoc, name));
            newSigArgs.emplace_back(ASTUtil::dupType(memberType));
            auto argName = ast::MK::Local(symLoc, name);
            newArgs.emplace_back(ast::MK::OptionalParam(symLoc, move(argName), ast::MK::Nil(symLoc)));
        }

        // Build initialize args (only when user provided initialize)
        if (initialize.has_value()) {
            initializeArgs.emplace_back(ast::MK::KeywordArg(symLoc, name));
            initializeSigArgs.emplace_back(ast::MK::Symbol(symLoc, name));
            initializeSigArgs.emplace_back(ASTUtil::dupType(memberType));
        }
    }

    // Generate self.new (only when no user-provided initialize with sig)
    if (newArgs.size() > 0) {
        body.emplace_back(ast::MK::Sig(loc, move(newSigArgs), ast::MK::AttachedClass(loc)));
        ast::MethodDef::Flags flags;
        flags.isSelfMethod = true;
        body.emplace_back(ast::MK::SyntheticMethod(loc, loc, core::Names::new_(), move(newArgs),
                                                   ast::MK::RaiseUnimplemented(loc), flags));
    }

    // Generate initialize (when user provided one or as default)
    if (initializeArgs.size() > 0) {
        body.emplace_back(ast::MK::SigVoid(loc, move(initializeSigArgs)));
        body.emplace_back(ast::MK::SyntheticMethod(loc, loc, core::Names::initialize(), move(initializeArgs),
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
