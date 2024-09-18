#include "MethodTypeTranslator.h"
#include "TypeTranslator.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/GlobalState.h"
#include "core/Names.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

// Add this hash function at the top of the file, outside of any namespace
constexpr unsigned int hash(const char *str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

namespace sorbet::rbs {

namespace {

// sorbet::ast::ExpressionPtr translateParams() {
//     Send::ARGS_store paramsStore;

//     for (long i = 0; i < RARRAY_LEN(params); i++) {
//         auto &methodArg = methodDef->args[i];

//         auto ident = ast::cast_tree<ast::UnresolvedIdent>(methodArg);
//         if (ident == nullptr) {
//             // TODO raise error
//             continue;
//         }

//         VALUE param = rb_ary_entry(params, i);
//         VALUE paramType = rb_funcall(param, rb_intern("type"), 0);

//         sigArgs.emplace_back(ast::MK::Symbol(methodArg.loc(), ident->name));
//         sigArgs.emplace_back(TypeTranslator::toRBI(ctx, paramType, methodArg.loc()));
//     }

//     return params;
// }

// sorbet::ast::ExpressionPtr translateParam(core::MutableContext ctx, VALUE param, ast::ExpressionPtr methodArg) {
//     auto ident = ast::cast_tree<ast::UnresolvedIdent>(methodArg);
//     if (ident == nullptr) {
//         // TODO raise error
//         continue;
//     }

//     VALUE paramType = rb_funcall(param, rb_intern("type"), 0);
//     auto translatedType = TypeTranslator::toRBI(ctx, paramType, methodArg.loc());

//     sigArgs.emplace_back(ast::MK::Symbol(methodArg.loc(), ident->name));
//     sigArgs.emplace_back(std::move(translatedType));
// }

} // namespace

sorbet::ast::ExpressionPtr MethodTypeTranslator::toRBI(core::MutableContext ctx, core::LocOffsets docLoc,
                                                       sorbet::ast::MethodDef *methodDef, VALUE methodType) {
    // std::cout << "METHOD DEF: " << methodDef->showRaw(ctx) << std::endl;

    // TODO raise error if methodType is not a MethodType
    // std::cout << rb_obj_classname(methodType) << std::endl;
    // rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);
    VALUE requiredPositionalsValue = rb_funcall(functionType, rb_intern("required_positionals"), 0);
    VALUE optionalPositionalsValue = rb_funcall(functionType, rb_intern("optional_positionals"), 0);
    VALUE restPositionalsValue = rb_funcall(functionType, rb_intern("rest_positionals"), 0);
    VALUE requiredKeywordsValue = rb_funcall(functionType, rb_intern("required_keywords"), 0);
    VALUE optionalKeywordsValue = rb_funcall(functionType, rb_intern("optional_keywords"), 0);
    VALUE restKeywordsValue = rb_funcall(functionType, rb_intern("rest_keywords"), 0);
    VALUE blockValue = rb_funcall(methodType, rb_intern("block"), 0);

    auto requiredPositionalsIndex = 0;
    auto optionalPositionalsIndex = 0;

    Send::ARGS_store sigArgs;
    for (int i = 0; i < methodDef->args.size(); i++) {
        auto &methodArg = methodDef->args[i];

        core::NameRef argName;
        ast::ExpressionPtr argType;

        // std::cout << "ARG: " << methodArg.showRaw(ctx) << std::endl;

        typecase(
            methodArg,
            [&](const ast::UnresolvedIdent &p) {
                argName = p.name;
                auto argValue = rb_ary_entry(requiredPositionalsValue, requiredPositionalsIndex);
                argType = TypeTranslator::toRBI(ctx, rb_funcall(argValue, rb_intern("type"), 0), methodArg.loc());
                requiredPositionalsIndex++;
            },
            [&](const ast::OptionalArg &p) {
                typecase(
                    p.expr,
                    [&](const ast::UnresolvedIdent &p) {
                        argName = p.name;
                        auto argValue = rb_ary_entry(optionalPositionalsValue, optionalPositionalsIndex);
                        argType = ast::MK::Nilable(
                            methodArg.loc(),
                            TypeTranslator::toRBI(ctx, rb_funcall(argValue, rb_intern("type"), 0), methodArg.loc()));
                        optionalPositionalsIndex++;
                    },
                    [&](const ast::KeywordArg &p) {
                        auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                        if (argIdent != nullptr) {
                            argName = argIdent->name;
                            VALUE key = ID2SYM(rb_intern(argName.show(ctx).c_str()));
                            auto argValue = rb_hash_aref(optionalKeywordsValue, key);
                            argType = ast::MK::Nilable(
                                methodArg.loc(), TypeTranslator::toRBI(ctx, rb_funcall(argValue, rb_intern("type"), 0),
                                                                       methodArg.loc()));
                        }
                    },
                    [&](const ast::ExpressionPtr &p) {
                        std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
                    });
            },
            [&](const ast::RestArg &p) {
                typecase(
                    p.expr,
                    [&](const ast::UnresolvedIdent &p) {
                        argName = p.name;
                        argType = TypeTranslator::toRBI(ctx, rb_funcall(restPositionalsValue, rb_intern("type"), 0),
                                                        methodArg.loc());
                    },
                    [&](const ast::KeywordArg &p) {
                        auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                        if (argIdent != nullptr) {
                            argName = argIdent->name;
                            argType = TypeTranslator::toRBI(ctx, rb_funcall(restKeywordsValue, rb_intern("type"), 0),
                                                            methodArg.loc());
                        }
                    },
                    [&](const ast::ExpressionPtr &p) {
                        std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
                    });
            },
            // TODO: Access the hash
            [&](const ast::KeywordArg &p) {
                auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                if (argIdent != nullptr) {
                    argName = argIdent->name;
                    VALUE key = ID2SYM(rb_intern(argName.show(ctx).c_str()));
                    auto argValue = rb_hash_aref(requiredKeywordsValue, key);
                    argType = TypeTranslator::toRBI(ctx, rb_funcall(argValue, rb_intern("type"), 0), methodArg.loc());
                }
            },
            [&](const ast::BlockArg &p) {
                auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                if (argIdent != nullptr) {
                    argName = argIdent->name;
                }
                if (blockValue != Qnil) {
                    argType = TypeTranslator::toRBI(ctx, rb_funcall(blockValue, rb_intern("type"), 0), methodArg.loc());
                }
            },
            [&](const ast::ExpressionPtr &p) { std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl; });

        if (argType == nullptr) {
            // std::cout << "NIL ARG: " << methodArg.showRaw(ctx) << std::endl;
            continue;
        }

        if (!argName.exists()) {
            std::cout << "MISSING ARG NAME: " << methodArg.showRaw(ctx) << std::endl;
            // TODO raise error
            continue;
        } else {
            // std::cout << "ARG NAME: " << argName.show(ctx) << std::endl;
        }

        sigArgs.emplace_back(ast::MK::Symbol(methodArg.loc(), argName));
        sigArgs.emplace_back(std::move(argType));
    }

    // visitParameterList(parameterList);
    //  (store.emplace_back(std::forward<Args>(args)), ...);

    // auto sigArgs = ast::MK::SendArgs(ast::MK::Symbol(loc, core::Names::arg0()), ast::MK::Untyped(loc),
    //                                 ast::MK::Symbol(loc, core::Names::blkArg()),
    //                                 ast::MK::Nilable(loc, ast::MK::Constant(loc, core::Symbols::Proc())));

    // Visit return type
    VALUE rbsReturnType = rb_funcall(functionType, rb_intern("return_type"), 0);
    auto rbiReturnType = TypeTranslator::toRBI(ctx, rbsReturnType, methodDef->loc);

    return ast::MK::Sig(docLoc, std::move(sigArgs), std::move(rbiReturnType));
}

} // namespace sorbet::rbs
