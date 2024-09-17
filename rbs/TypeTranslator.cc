#include "TypeTranslator.h"
#include "ast/ast.h"
#include "ast/Helpers.h"
#include "core/Names.h"
#include "core/GlobalState.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

namespace sorbet::rbs {

namespace {

// TODO: do beter than this
constexpr unsigned int hash(const char* str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

sorbet::ast::ExpressionPtr typeNameType(core::MutableContext ctx, VALUE typeName, core::LocOffsets loc) {
    VALUE typeNamespace = rb_funcall(typeName, rb_intern("namespace"), 0);
    VALUE typePath = rb_funcall(typeNamespace, rb_intern("path"), 0);

    auto parent = ast::MK::EmptyTree();
    if (!NIL_P(typePath)) {
        long pathLength = RARRAY_LEN(typePath);

        for (long i = 0; i < pathLength; i++) {
            VALUE pathName = rb_ary_entry(typePath, i);
            VALUE pathNameToS = rb_funcall(pathName, rb_intern("to_s"), 0);

            std::string pathNameStr(RSTRING_PTR(pathNameToS));
            auto pathNameConst = ctx.state.enterNameConstant(pathNameStr);

            parent = ast::MK::UnresolvedConstant(loc, std::move(parent), pathNameConst);
        }
    }

    VALUE nameValue = rb_funcall(typeName, rb_intern("name"), 0);
    VALUE nameToS = rb_funcall(nameValue, rb_intern("to_s"), 0);
    std::string nameStr(RSTRING_PTR(nameToS));
    auto nameConstant = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, std::move(parent), nameConstant);
}

sorbet::ast::ExpressionPtr classInstanceType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE typeName = rb_funcall(type, rb_intern("name"), 0);

    return typeNameType(ctx, typeName, loc);
}

sorbet::ast::ExpressionPtr classSingletonType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE typeName = rb_funcall(type, rb_intern("name"), 0);
    auto innerType = typeNameType(ctx, typeName, loc);

    return ast::MK::ClassOf(loc, std::move(innerType));
}

sorbet::ast::ExpressionPtr unionType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE types = rb_funcall(type, rb_intern("types"), 0);
    auto typesStore = Send::ARGS_store();
    for (long i = 0; i < RARRAY_LEN(types); i++) {
        VALUE typeValue = rb_ary_entry(types, i);
        auto innerType = TypeTranslator::toRBI(ctx, typeValue, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Any(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr intersectionType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE types = rb_funcall(type, rb_intern("types"), 0);
    auto typesStore = Send::ARGS_store();
    for (long i = 0; i < RARRAY_LEN(types); i++) {
        VALUE typeValue = rb_ary_entry(types, i);
        auto innerType = TypeTranslator::toRBI(ctx, typeValue, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::All(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr optionalType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE innerValue = rb_funcall(type, rb_intern("type"), 0);
    auto innerType = TypeTranslator::toRBI(ctx, innerValue, loc);

    return ast::MK::Nilable(loc, std::move(innerType));
}

sorbet::ast::ExpressionPtr voidType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    auto cSorbet = ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::Sorbet());
    auto cPrivate = ast::MK::UnresolvedConstant(loc, std::move(cSorbet), core::Names::Constants::Private());
    auto cStatic = ast::MK::UnresolvedConstant(loc, std::move(cPrivate), core::Names::Constants::Static());

    return ast::MK::UnresolvedConstant(loc, std::move(cStatic), core::Names::Constants::Void());
}

} // namespace

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    rb_p(type);
    const char* className = rb_obj_classname(type);
    // TODO: handle errors

    switch (hash(className)) {
        case hash("RBS::Types::ClassInstance"):
            return classInstanceType(ctx, type, loc);
        case hash("RBS::Types::ClassSingleton"):
            return classSingletonType(ctx, type, loc);
        case hash("RBS::Types::Optional"):
            return optionalType(ctx, type, loc);
        case hash("RBS::Types::Union"):
            return unionType(ctx, type, loc);
        case hash("RBS::Types::Intersection"):
            return intersectionType(ctx, type, loc);
        case hash("RBS::Types::Bases::Self"):
            return ast::MK::SelfType(loc);
        case hash("RBS::Types::Bases::Instance"):
            return ast::MK::AttachedClass(loc);
        case hash("RBS::Types::Bases::Class"):
            // TODO: get around type? error
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Bool"):
            return ast::MK::T_Boolean(loc);
        case hash("RBS::Types::Bases::Nil"):
            return ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::NilClass());
        case hash("RBS::Types::Bases::Top"):
            return ast::MK::Anything(loc);
        case hash("RBS::Types::Bases::Bottom"):
            return ast::MK::NoReturn(loc);
        case hash("RBS::Types::Bases::Void"):
            return voidType(ctx, type, loc);

        default:
            return ast::MK::Untyped(loc);
    }
}

} // namespace sorbet::rbs
