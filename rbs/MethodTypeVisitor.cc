#include "MethodTypeVisitor.h"
#include "ast/ast.h"
#include "ast/Helpers.h"
#include "core/Names.h"
#include "core/GlobalState.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

// Add this hash function at the top of the file, outside of any namespace
constexpr unsigned int hash(const char* str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

namespace sorbet::rbs {

MethodTypeVisitor::MethodTypeVisitor(core::MutableContext ctx, sorbet::ast::MethodDef *methodDef) : ctx(ctx), methodDef(methodDef) {}

sorbet::ast::ExpressionPtr MethodTypeVisitor::visitMethodType(VALUE methodType) {
    auto loc = methodDef->loc;

    // TODO raise error if methodType is not a MethodType
    // rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);
    // rb_p(functionType);

    // auto sig = createSig();

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

        auto translatedType = translateType(paramType);
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
    auto rbiReturnType = translateType(rbsReturnType);

    return ast::MK::Sig(loc, std::move(sigArgs), std::move(rbiReturnType));
}

sorbet::ast::ExpressionPtr MethodTypeVisitor::createSig() {
    auto loc = methodDef->loc;

    auto sigArgs = ast::MK::SendArgs(ast::MK::Symbol(loc, core::Names::arg0()), ast::MK::Untyped(loc),
                                     ast::MK::Symbol(loc, core::Names::blkArg()),
                                     ast::MK::Nilable(loc, ast::MK::Constant(loc, core::Symbols::Proc())));

    auto sig = ast::MK::Sig(loc, std::move(sigArgs), ast::MK::Untyped(loc));

    return sig;
}

void MethodTypeVisitor::visitParameterList(VALUE parameterList) {
    // Implement parameter list traversal
    // This will depend on the structure of the RBS parameter list
    // You may need to add more methods to handle different parameter types
}

void MethodTypeVisitor::visitType(VALUE type) {
    // Implement type traversal
    // This will depend on the structure of the RBS type
    // You may need to add more methods to handle different RBS types
}

sorbet::ast::ExpressionPtr MethodTypeVisitor::translateType(VALUE type) {
    // rb_p(type);

    auto loc = methodDef->loc;
    const char* className = rb_obj_classname(type);

    switch (hash(className)) {
        case hash("RBS::Types::Bases::Any"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Void"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Bool"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Integer"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::String"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Symbol"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Nil"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Union"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Optional"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::ClassInstance"):
            return translateClassInstanceType(loc, type);
        case hash("RBS::Types::ClassSingleton"):
            return ast::MK::Untyped(loc);
        default:
            return ast::MK::Untyped(loc);
    }
}

sorbet::ast::ExpressionPtr MethodTypeVisitor::translateClassInstanceType(core::LocOffsets loc, VALUE type) {
    VALUE nameSymbol = rb_funcall(type, rb_intern("name"), 0);
    VALUE nameString = rb_funcall(nameSymbol, rb_intern("to_s"), 0);
    const char* nameChars = RSTRING_PTR(nameString);
    std::string nameStr(nameChars);
    auto name = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), name);
}



// Implement more methods as needed to handle different RBS node types

} // namespace sorbet::rbs
