#include "TypeTranslator.h"
#include "absl/strings/strip.h"
#include "ast/ast.h"
#include "ast/Helpers.h"
#include "core/Names.h"
#include "core/GlobalState.h"
#include <cstring>
#include <functional>

using namespace sorbet::ast;

// TODO: do beter than this
constexpr unsigned int hash(const char* str) {
    return *str ? static_cast<unsigned int>(*str) + 33 * hash(str + 1) : 5381;
}

namespace sorbet::rbs {

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    const char* className = rb_obj_classname(type);
    // TODO: handle errors

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
            return classInstanceType(ctx, type, loc);
        case hash("RBS::Types::ClassSingleton"):
            return ast::MK::Untyped(loc);
        default:
            return ast::MK::Untyped(loc);
    }
}

sorbet::ast::ExpressionPtr TypeTranslator::classInstanceType(core::MutableContext ctx, VALUE type, core::LocOffsets loc) {
    VALUE nameSymbol = rb_funcall(type, rb_intern("name"), 0);
    VALUE nameString = rb_funcall(nameSymbol, rb_intern("to_s"), 0);
    const char* nameChars = RSTRING_PTR(nameString);

    std::string nameStr(nameChars);
    nameStr = absl::StripPrefix(nameStr, "::");

    auto name = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), name);
}



// Implement more methods as needed to handle different RBS node types

} // namespace sorbet::rbs
