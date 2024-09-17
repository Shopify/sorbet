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

sorbet::ast::ExpressionPtr MethodTypeTranslator::toRBI(core::MutableContext ctx, sorbet::ast::MethodDef *methodDef, VALUE methodType) {
    auto loc = methodDef->loc;

    // TODO raise error if methodType is not a MethodType
    // rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);
    // rb_p(functionType);

    // // Visit parameter list
    VALUE requiredPositionals = rb_funcall(functionType, rb_intern("required_positionals"), 0);
    // rb_p(requiredPositionals);

    long numRequiredPositionals = RARRAY_LEN(requiredPositionals);

    Send::ARGS_store sigArgs;
    for (long i = 0; i < numRequiredPositionals; i++) {
        auto &methodArg = methodDef->args[i];

        auto ident = ast::cast_tree<ast::UnresolvedIdent>(methodArg);
        if (ident == nullptr) {
            // TODO raise error
            continue;
        }
        // auto *methodArgName = &methodArg;

        // typecase(
        //     *methodArg,
        //     [&](const ast::RestArg &rest) {
        //         methodArgName = &rest.expr;
        //     },
        //     [&](const ast::KeywordArg &kw) {
        //         methodArgName = &kw.expr;
        //     },
        //     [&](const ast::OptionalArg &opt) {
        //         methodArgName = &opt.expr;
        //     },
        //     [&](const ast::BlockArg &blk) {
        //         methodArgName = &blk.expr;
        //     },
        //     [&](const ast::ShadowArg &shadow) {
        //         methodArgName = &shadow.expr;
        //     },
        //     [&](const ast::Local &local) {
        //         methodArgName = nullptr;
        //     }
        // );

        VALUE param = rb_ary_entry(requiredPositionals, i);
        // // VALUE paramName = rb_funcall(param, rb_intern("name"), 0);
        VALUE paramType = rb_funcall(param, rb_intern("type"), 0);
        // rb_p(paramType);

        auto translatedType = TypeTranslator::toRBI(ctx, paramType, methodArg.loc());
        // std::cout << "translatedType: " << translatedType << std::endl;

        sigArgs.emplace_back(ast::MK::Symbol(methodArg.loc(), ident->name));
        sigArgs.emplace_back(std::move(translatedType));
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
