#include "MethodTypeTranslator.h"
#include "TypeTranslator.h"
#include "ast/Helpers.h"
#include "core/errors/rewriter.h"
#include "core/GlobalState.h"
#include "absl/strings/escaping.h"

using namespace sorbet::ast;

// Add this hash function at the top of the file, outside of any namespace
constexpr unsigned int hash(const char *str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

namespace sorbet::rbs {

namespace {

core::NameRef expressionName(core::MutableContext ctx, const ast::ExpressionPtr *expr) {
    core::NameRef name;

    typecase(
        *expr, [&](const ast::UnresolvedIdent &p) { name = p.name; },
        [&](const ast::OptionalArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::RestArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::KeywordArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::BlockArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::ExpressionPtr &p) {
            if (auto e = ctx.beginError(expr->loc(), core::errors::Rewriter::BadAttrArg)) {
                e.setHeader("Unexpected expression type: {}", p.showRaw(ctx));
            }

            name = ctx.state.enterNameUTF8("<error>");
        });

    return name;
}

// TODO: merge with AttrReader.cc
std::pair<core::NameRef, core::LocOffsets> getName(core::MutableContext ctx, ast::ExpressionPtr &name) {
    core::LocOffsets loc;
    core::NameRef res;
    if (auto *lit = ast::cast_tree<ast::Literal>(name)) {
        if (lit->isSymbol()) {
            res = lit->asSymbol();
            loc = lit->loc;
            ENFORCE(ctx.locAt(loc).exists());
            ENFORCE(ctx.locAt(loc).source(ctx).value().size() > 1 && ctx.locAt(loc).source(ctx).value()[0] == ':');
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos()};
        } else if (lit->isString()) {
            core::NameRef nameRef = lit->asString();
            auto shortName = nameRef.shortName(ctx);
            bool validAttr = (isalpha(shortName.front()) || shortName.front() == '_') &&
                             absl::c_all_of(shortName, [](char c) { return isalnum(c) || c == '_'; });
            if (validAttr) {
                res = nameRef;
            } else {
                if (auto e = ctx.beginError(name.loc(), core::errors::Rewriter::BadAttrArg)) {
                    e.setHeader("Bad attribute name \"{}\"", absl::CEscape(shortName));
                }
                res = core::Names::empty();
            }
            loc = lit->loc;
            DEBUG_ONLY({
                auto l = ctx.locAt(loc);
                ENFORCE(l.exists());
                auto source = l.source(ctx).value();
                ENFORCE(source.size() > 2);
                ENFORCE(source[0] == '"' || source[0] == '\'');
                auto lastChar = source[source.size() - 1];
                ENFORCE(lastChar == '"' || lastChar == '\'');
            });
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos() - 1};
        }
    }
    if (!res.exists()) {
        if (auto e = ctx.beginError(name.loc(), core::errors::Rewriter::BadAttrArg)) {
            e.setHeader("arg must be a Symbol or String");
        }
    }
    return std::make_pair(res, loc);
}

struct RBSArg {
    core::LocOffsets loc;
    rbs_ast_symbol_t *name;
    rbs_node_t *type;
    bool optional;
};

void collectArgs(core::MutableContext ctx, core::LocOffsets docLoc, rbs_node_list_t *field, std::vector<RBSArg> &args,
                 bool optional) {
    for (rbs_node_list_node_t *list_node = field->head; list_node != nullptr; list_node = list_node->next) {
        if (list_node->node->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(list_node->node));
            }
            continue;
        }

        auto node = (rbs_types_function_param_t *)list_node->node;
        auto loc = TypeTranslator::locOffsets(docLoc, node->location);
        auto arg = RBSArg{loc, node->name, node->type, optional};
        args.emplace_back(arg);
    }
}

void collectKeywords(core::MutableContext ctx, core::LocOffsets docLoc, rbs_hash_t *field, std::vector<RBSArg> &args,
                     bool optional) {
    for (rbs_hash_node_t *hash_node = field->head; hash_node != nullptr; hash_node = hash_node->next) {
        if (hash_node->key->type != RBS_AST_SYMBOL) {
            if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(hash_node->key));
            }
            continue;
        }

        if (hash_node->value->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(hash_node->value));
            }
            continue;
        }

        rbs_ast_symbol_t *keyNode = (rbs_ast_symbol_t *) hash_node->key;
        rbs_types_function_param_t *valueNode = (rbs_types_function_param_t *) hash_node->value;
        auto arg = RBSArg{docLoc, keyNode, valueNode->type, optional};
        args.emplace_back(arg);
    }
}

} // namespace

sorbet::ast::ExpressionPtr MethodTypeTranslator::methodSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                                 sorbet::ast::MethodDef *methodDef,
                                                                 rbs_methodtype_t *methodType,
                                                                 std::vector<RBSAnnotation> annotations) {
    if (methodType->type->type != RBS_TYPES_FUNCTION) {
        if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Unexpected node type: {}", rbs_node_type_name(methodType->type));
        }
        return ast::MK::Untyped(docLoc);
    }

    rbs_types_function_t *functionType = (rbs_types_function_t *)methodType->type;
    Send::ARGS_store sigArgs;
    std::vector<RBSArg> args;

    collectArgs(ctx, docLoc, functionType->required_positionals, args, false);

    rbs_node_list_t *optionalPositionals = functionType->optional_positionals;
    if (optionalPositionals && optionalPositionals->length > 0) {
        collectArgs(ctx, docLoc, optionalPositionals, args, false);
    }

    rbs_node_t *restPositionals = functionType->rest_positionals;
    if (restPositionals) {
        if (restPositionals->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(restPositionals));
            }
        } else {
            auto node = (rbs_types_function_param_t *)restPositionals;
            auto loc = TypeTranslator::locOffsets(docLoc, node->location);
            auto arg = RBSArg{loc, node->name, node->type, false};
            args.emplace_back(arg);
        }
    }

    rbs_node_list_t *trailingPositionals = functionType->trailing_positionals;
    if (trailingPositionals && trailingPositionals->length > 0) {
        collectArgs(ctx, docLoc, trailingPositionals, args, false);
    }

    collectKeywords(ctx, docLoc, functionType->required_keywords, args, false);
    collectKeywords(ctx, docLoc, functionType->optional_keywords, args, false);

    rbs_node_t *restKeywords = functionType->rest_keywords;
    if (restKeywords) {
        if (restKeywords->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(restKeywords));
            }
        } else {
            auto node = (rbs_types_function_param_t *)restKeywords;
            auto loc = TypeTranslator::locOffsets(docLoc, node->location);
            auto arg = RBSArg{loc, node->name, node->type, false};
            args.emplace_back(arg);
        }
    }

    rbs_types_block_t *block = methodType->block;
    if (block) {
        // TODO: RBS doesn't have location on blocks yet
        auto arg = RBSArg{docLoc, nullptr, (rbs_node_t *) block, !block->required};
        args.emplace_back(arg);
    }

    for (int i = 0; i < args.size(); i++) {
        auto &arg = args[i];
        core::NameRef name;
        auto nameSymbol = arg.name;

        if (nameSymbol) {
            rbs_constant_t *nameConstant = rbs_constant_pool_id_to_constant(fake_constant_pool, nameSymbol->constant_id);
            std::string nameStr(nameConstant->start);
            name = ctx.state.enterNameUTF8(nameStr);
        } else {
            // The RBS arg is not named in the signature, so we look at the method definition
            auto &methodArg = methodDef->args[i];
            name = expressionName(ctx, &methodArg);
        }

        auto type = TypeTranslator::toRBI(ctx, arg.type, docLoc);
        if (arg.optional) {
            type = ast::MK::Nilable(arg.loc, std::move(type));
        }

        sigArgs.emplace_back(ast::MK::Symbol(arg.loc, name));
        sigArgs.emplace_back(std::move(type));
    }

    auto sigBuilder = ast::MK::Self(docLoc);

    for (auto &annotation : annotations) {
        if (annotation.string == "@abstract") {
            sigBuilder = ast::MK::Send0(annotation.loc, std::move(sigBuilder), core::Names::abstract(), annotation.loc);
        } else if (annotation.string == "@override") {
            sigBuilder =
                ast::MK::Send0(annotation.loc, std::move(sigBuilder), core::Names::override_(), annotation.loc);
        }
    }

    ENFORCE(sigArgs.size() % 2 == 0, "Sig params must be arg name/type pairs");

    if (sigArgs.size() > 0) {
        sigBuilder = ast::MK::Send(docLoc, std::move(sigBuilder), core::Names::params(), docLoc, 0, std::move(sigArgs));
    }

    rbs_node_t *returnValue = functionType->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        auto loc = TypeTranslator::locOffsets(docLoc, ((rbs_types_bases_void_t *)returnValue)->location);
        sigBuilder = ast::MK::Send0(loc, std::move(sigBuilder), core::Names::void_(), loc);
    } else {
        auto returnType = TypeTranslator::toRBI(ctx, returnValue, docLoc);
        sigBuilder =
            ast::MK::Send1(docLoc, std::move(sigBuilder), core::Names::returns(), docLoc, std::move(returnType));
    }

    auto sig =
        ast::MK::Send1(docLoc, ast::MK::Constant(docLoc, core::Symbols::Sorbet_Private_Static()), core::Names::sig(),
                       docLoc, ast::MK::Constant(docLoc, core::Symbols::T_Sig_WithoutRuntime()));
    auto sigSend = ast::cast_tree<ast::Send>(sig);
    sigSend->setBlock(ast::MK::Block0(docLoc, std::move(sigBuilder)));
    sigSend->flags.isRewriterSynthesized = true;

    return sig;
}

sorbet::ast::ExpressionPtr MethodTypeTranslator::attrSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                               sorbet::ast::Send *send, rbs_node_t *attrType,
                                                               std::vector<RBSAnnotation> annotations) {
    auto sigBuilder = ast::MK::Self(docLoc);

    if (send->fun == core::Names::attrWriter()) {
        ENFORCE(send->numPosArgs() >= 1);
        auto name = getName(ctx, send->getPosArg(0));

        Send::ARGS_store sigArgs;
        sigArgs.emplace_back(ast::MK::Symbol(name.second, name.first));
        sigArgs.emplace_back(TypeTranslator::toRBI(ctx, attrType, docLoc));
        sigBuilder = ast::MK::Send(docLoc, std::move(sigBuilder), core::Names::params(), docLoc, 0, std::move(sigArgs));
    }

    auto returnType = TypeTranslator::toRBI(ctx, attrType, docLoc);
    sigBuilder = ast::MK::Send1(docLoc, std::move(sigBuilder), core::Names::returns(), docLoc, std::move(returnType));

    auto sig =
        ast::MK::Send1(docLoc, ast::MK::Constant(docLoc, core::Symbols::Sorbet_Private_Static()), core::Names::sig(),
                       docLoc, ast::MK::Constant(docLoc, core::Symbols::T_Sig_WithoutRuntime()));
    auto sigSend = ast::cast_tree<ast::Send>(sig);
    sigSend->setBlock(ast::MK::Block0(docLoc, std::move(sigBuilder)));
    sigSend->flags.isRewriterSynthesized = true;

    return sig;
}

} // namespace sorbet::rbs
