#include "MethodTypeTranslator.h"
#include "TypeTranslator.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/GlobalState.h"
#include "core/Names.h"
#include "core/errors/rewriter.h"

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

        VALUE argValue;
        core::NameRef argName;
        bool asNilable = false;
        // std::cout << "ARG: " << methodArg.showRaw(ctx) << std::endl;

        typecase(
            methodArg,
            [&](const ast::UnresolvedIdent &p) {
                argValue = rb_ary_entry(requiredPositionalsValue, requiredPositionalsIndex);
                if (argValue == Qnil) {
                    std::cout << "ARG VALUE NIL" << std::endl;
                    core::LocOffsets offset{docLoc.beginLoc, docLoc.endLoc};
                    if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
                        e.setHeader("Malformed `{}`. Type not specified for required positional `{}`", "sig",
                                    p.name.show(ctx));
                    }
                    return;
                }

                auto rbsName = rb_funcall(argValue, rb_intern("name"), 0);
                if (rbsName == Qnil) {
                    argName = p.name;
                } else {
                    auto rbsString = rb_funcall(rbsName, rb_intern("to_s"), 0);
                    const char *nameStr = StringValueCStr(rbsString);
                    argName = ctx.state.enterNameUTF8(nameStr);
                    rb_p(rbsName);
                }
                requiredPositionalsIndex++;
            },
            [&](const ast::OptionalArg &p) {
                typecase(
                    p.expr,
                    [&](const ast::UnresolvedIdent &p) {
                        argName = p.name;
                        argValue = rb_ary_entry(optionalPositionalsValue, optionalPositionalsIndex);
                        asNilable = true;
                        optionalPositionalsIndex++;
                        if (argValue == Qnil) {
                            core::LocOffsets offset{docLoc.beginLoc + 2, docLoc.beginLoc + 2};
                            if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
                                e.setHeader("Malformed `{}`. Type not specified for argument `{}`", "sig",
                                            argName.show(ctx));
                            }
                            return;
                        }
                    },
                    [&](const ast::KeywordArg &p) {
                        auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                        if (argIdent != nullptr) {
                            argName = argIdent->name;
                            VALUE key = ID2SYM(rb_intern(argName.show(ctx).c_str()));
                            argValue = rb_hash_aref(optionalKeywordsValue, key);
                            asNilable = true;
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
                        argValue = restPositionalsValue;
                        argName = p.name;
                    },
                    [&](const ast::KeywordArg &p) {
                        auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                        if (argIdent != nullptr) {
                            argName = argIdent->name;
                        }
                        argValue = restKeywordsValue;
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
                    argValue = rb_hash_aref(requiredKeywordsValue, key);
                }
            },
            [&](const ast::BlockArg &p) {
                auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                if (argIdent != nullptr) {
                    argName = argIdent->name;
                }
                argValue = blockValue;
            },
            [&](const ast::ExpressionPtr &p) { std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl; });

        if (!argName.exists()) {
            std::cout << "MISSING ARG NAME: " << methodArg.showRaw(ctx) << std::endl;
            // TODO raise error
            continue;
        } else {
            // std::cout << "ARG NAME: " << argName.show(ctx) << std::endl;
        }

        if (argValue == Qnil) {
            // if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
            //     e.addErrorNote("Malformed `sig`. Type not specified for argument `{}`", argName.show(ctx));
            // }
            continue;
        }

        auto argTypeValue = rb_funcall(argValue, rb_intern("type"), 0);
        auto argType = TypeTranslator::toRBI(ctx, argTypeValue, methodArg.loc());
        if (asNilable) {
            argType = ast::MK::Nilable(methodArg.loc(), std::move(argType));
        }

        VALUE rbsLoc = rb_funcall(argValue, rb_intern("location"), 0);
        VALUE rbsStartColumnValue = rb_funcall(rbsLoc, rb_intern("start_column"), 0);
        int rbsStartColumn = NUM2INT(rbsStartColumnValue);
        VALUE rbsEndColumnValue = rb_funcall(rbsLoc, rb_intern("end_column"), 0);
        int rbsEndColumn = NUM2INT(rbsEndColumnValue);
        core::LocOffsets rbiOffsets{docLoc.beginLoc + rbsStartColumn + 2, docLoc.beginLoc + rbsEndColumn + 2};
        // std::cout << "DOC LOC: " << docLoc.showRaw(ctx) << std::endl;
        // std::cout << "RBI OFFSETS: " << rbiOffsets.showRaw(ctx) << std::endl;

        sigArgs.emplace_back(ast::MK::Symbol(rbiOffsets, argName));
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
