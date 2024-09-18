#include "TypeTranslator.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/GlobalState.h"
#include "core/Names.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

namespace sorbet::rbs {

namespace {

// TODO: do beter than this
constexpr unsigned int hash(const char *str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

sorbet::ast::ExpressionPtr typeNameType(core::MutableContext ctx, VALUE typeName, core::LocOffsets loc) {
    VALUE typeNamespace = rb_funcall(typeName, rb_intern("namespace"), 0);
    VALUE typePath = rb_funcall(typeNamespace, rb_intern("path"), 0);

    auto parent = ast::MK::EmptyTree();
    if (!NIL_P(typePath)) {
        for (long i = 0; i < RARRAY_LEN(typePath); i++) {
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

    if (NIL_P(typePath) || RARRAY_LEN(typePath) == 0) {
        if (nameStr == "Array") {
            return ast::MK::T_Array(loc);
        } else if (nameStr == "Hash") {
            return ast::MK::T_Hash(loc);
        }
    }

    auto nameConstant = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, std::move(parent), nameConstant);
}

sorbet::ast::ExpressionPtr classInstanceType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE typeValue = rb_funcall(type, rb_intern("name"), 0);
    auto typeConstant = typeNameType(ctx, typeValue, loc);

    VALUE argsValue = rb_funcall(type, rb_intern("args"), 0);
    if (!NIL_P(argsValue) && RARRAY_LEN(argsValue) > 0) {
        auto argsStore = Send::ARGS_store();
        for (long i = 0; i < RARRAY_LEN(argsValue); i++) {
            VALUE argValue = rb_ary_entry(argsValue, i);
            auto argType = TypeTranslator::toRBI(ctx, argValue, loc);
            argsStore.emplace_back(std::move(argType));
        }

        return ast::MK::Send(loc, std::move(typeConstant), core::Names::squareBrackets(), loc, argsStore.size(),
                             std::move(argsStore));
    }

    return typeConstant;
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

sorbet::ast::ExpressionPtr functionType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE requiredPositionalsValue = rb_funcall(type, rb_intern("required_positionals"), 0);
    VALUE returnValue = rb_funcall(type, rb_intern("return_type"), 0);
    auto returnType = TypeTranslator::toRBI(ctx, returnValue, loc);

    auto paramsStore = Send::ARGS_store();
    for (long i = 0; i < RARRAY_LEN(requiredPositionalsValue); i++) {
        auto argName = ctx.state.enterNameUTF8("arg" + std::to_string(i));
        paramsStore.emplace_back(ast::MK::Symbol(loc, argName));

        VALUE paramValue = rb_ary_entry(requiredPositionalsValue, i);
        VALUE paramType = rb_funcall(paramValue, rb_intern("type"), 0);
        auto innerType = TypeTranslator::toRBI(ctx, paramType, loc);
        paramsStore.emplace_back(std::move(innerType));
    }

    return ast::MK::T_Proc(loc, std::move(paramsStore), std::move(returnType));
}

sorbet::ast::ExpressionPtr procType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE typeValue = rb_funcall(type, rb_intern("type"), 0);
    return functionType(ctx, typeValue, loc);
}

sorbet::ast::ExpressionPtr tupleType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE types = rb_funcall(type, rb_intern("types"), 0);
    auto typesStore = Array::ENTRY_store();
    for (long i = 0; i < RARRAY_LEN(types); i++) {
        VALUE typeValue = rb_ary_entry(types, i);
        auto innerType = TypeTranslator::toRBI(ctx, typeValue, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Array(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr recordType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE entries = rb_funcall(type, rb_intern("all_fields"), 0);

    auto keysStore = Hash::ENTRY_store();
    auto valuesStore = Hash::ENTRY_store();

    VALUE keys = rb_funcall(entries, rb_intern("keys"), 0);
    for (long i = 0; i < RARRAY_LEN(keys); i++) {
        VALUE key = rb_ary_entry(keys, i);
        VALUE value = rb_ary_entry(rb_hash_aref(entries, key), 0);

        VALUE keyToS = rb_funcall(key, rb_intern("to_s"), 0);
        std::string keyStr(RSTRING_PTR(keyToS));
        auto keyName = ctx.state.enterNameUTF8(keyStr);
        auto keyLiteral =
            ast::MK::Literal(loc, core::make_type<core::NamedLiteralType>(core::Symbols::Symbol(), keyName));
        keysStore.emplace_back(std::move(keyLiteral));

        rb_p(value);
        auto innerType = TypeTranslator::toRBI(ctx, value, loc);
        valuesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Hash(loc, std::move(keysStore), std::move(valuesStore));
}

} // namespace

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    // rb_p(type);
    const char *className = rb_obj_classname(type);
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
        case hash("RBS::Types::Bases::Untyped"):
            return ast::MK::Untyped(loc);
        case hash("RBS::Types::Bases::Self"):
            return ast::MK::SelfType(loc);
        case hash("RBS::Types::Bases::Instance"):
            return ast::MK::AttachedClass(loc);
        case hash("RBS::Types::Bases::Class"):
            return ast::MK::Untyped(loc); // TODO: get around type? error
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
        case hash("RBS::Types::Proc"):
            return procType(ctx, type, loc);
        case hash("RBS::Types::Function"):
            return functionType(ctx, type, loc);
        case hash("RBS::Types::Tuple"):
            return tupleType(ctx, type, loc);
        case hash("RBS::Types::Record"):
            return recordType(ctx, type, loc);

        default:
            std::cout << "unknown type: " << className << std::endl;
            rb_p(type);
            return ast::MK::Untyped(loc);
    }
}

} // namespace sorbet::rbs
