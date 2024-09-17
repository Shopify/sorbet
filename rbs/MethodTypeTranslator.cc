#include "MethodTypeTranslator.h"
#include "absl/strings/strip.h"
#include "ast/ast.h"
#include "ast/Helpers.h"
#include "core/Names.h"
#include "core/GlobalState.h"
#include "TypeTranslator.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

// Add this hash function at the top of the file, outside of any namespace
constexpr unsigned int hash(const char* str) {
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

}

sorbet::ast::ExpressionPtr MethodTypeTranslator::toRBI(core::MutableContext ctx, sorbet::ast::MethodDef *methodDef, VALUE methodType) {
    auto loc = methodDef->loc;

    std::cout << "METHOD DEF: " << methodDef->showRaw(ctx) << std::endl;

    // TODO raise error if methodType is not a MethodType
    rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);
    VALUE requiredPositionalsValue = rb_funcall(functionType, rb_intern("required_positionals"), 0);
    VALUE optionalPositionalsValue = rb_funcall(functionType, rb_intern("optional_positionals"), 0);
    VALUE restPositionalsValue = rb_funcall(functionType, rb_intern("rest_positionals"), 0);
    VALUE requiredKeywordsValue = rb_funcall(functionType, rb_intern("required_keywords"), 0);
    // VALUE optionalKeywordsValue = rb_funcall(functionType, rb_intern("optional_keywords"), 0);
    // VALUE restKeywordsValue = rb_funcall(functionType, rb_intern("rest_keywords"), 0);
    // VALUE blockValue = rb_funcall(functionType, rb_intern("block"), 0);

    Send::ARGS_store sigArgs;
    auto argIndex = 0;

    for (int i = 0; i < methodDef->args.size(); i++) {
        auto &methodArg = methodDef->args[i];

        core::NameRef argName;
        VALUE arg;

        auto requiredPositionalsIndex = 0;
        auto optionalPositionalsIndex = 0;
        auto requiredKeywordsIndex = 0;
        typecase(
            methodArg,
            [&](const ast::UnresolvedIdent &p) {
                argName = p.name;
                arg = rb_ary_entry(requiredPositionalsValue, requiredPositionalsIndex);
                requiredPositionalsIndex++;
             },
             [&](const ast::OptionalArg &p) {
                auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                if(argIdent != nullptr) {
                    argName = argIdent->name;
                }
                arg = rb_ary_entry(optionalPositionalsValue, optionalPositionalsIndex);
                optionalPositionalsIndex++;
             },
             [&](const ast::RestArg &p) {
                auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
                if(argIdent != nullptr) {
                    argName = argIdent->name;
                }
                arg = restPositionalsValue;
             },
            //  [&](const ast::KeywordArg &p) {
            //     auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
            //     if(argIdent != nullptr) {
            //         argName = argIdent->name;
            //     }
            //     arg = rb_ary_entry(requiredKeywordsValue, requiredKeywordsIndex);
            //     requiredKeywordsIndex++;
            //  },
             [&](const ast::ExpressionPtr &p) {
                std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
             }
        );

        if (!argName.exists()) {
            std::cout << "MISSING ARG NAME: " << argIndex << std::endl;
            // TODO raise error
            continue;
        }

        sigArgs.emplace_back(ast::MK::Symbol(methodArg.loc(), argName));

        VALUE argType = rb_funcall(arg, rb_intern("type"), 0);
        sigArgs.emplace_back(TypeTranslator::toRBI(ctx, argType, methodArg.loc()));
    }

    //visitParameterList(parameterList);
    // (store.emplace_back(std::forward<Args>(args)), ...);

    // auto sigArgs = ast::MK::SendArgs(ast::MK::Symbol(loc, core::Names::arg0()), ast::MK::Untyped(loc),
    //                                 ast::MK::Symbol(loc, core::Names::blkArg()),
    //                                 ast::MK::Nilable(loc, ast::MK::Constant(loc, core::Symbols::Proc())));

    // Visit return type
    VALUE rbsReturnType = rb_funcall(functionType, rb_intern("return_type"), 0);
    auto rbiReturnType = TypeTranslator::toRBI(ctx, rbsReturnType, methodDef->loc);

    return ast::MK::Sig(loc, std::move(sigArgs), std::move(rbiReturnType));
}

} // namespace sorbet::rbs
