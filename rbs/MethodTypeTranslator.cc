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

core::NameRef expressionName(core::MutableContext ctx, const ast::ExpressionPtr *expr) {
    core::NameRef name;

    typecase(
        *expr, [&](const ast::UnresolvedIdent &p) { name = p.name; },
        [&](const ast::OptionalArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::RestArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::KeywordArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::BlockArg &p) { name = expressionName(ctx, &p.expr); },
        [&](const ast::ExpressionPtr &p) {
            std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
            name = ctx.state.enterNameUTF8("TMP");
        });

    return name;
}

struct RBSArg {
    core::LocOffsets loc;
    VALUE value;
    VALUE name;
    VALUE type;
    bool optional;
};

core::LocOffsets rbsLoc(core::LocOffsets docLoc, VALUE location) {
    int rbsStartColumn = NUM2INT(rb_funcall(location, rb_intern("start_column"), 0));
    int rbsEndColumn = NUM2INT(rb_funcall(location, rb_intern("end_column"), 0));
    return core::LocOffsets{docLoc.beginLoc + rbsStartColumn + 2, docLoc.beginLoc + rbsEndColumn + 2};
}

RBSArg rbsArg(core::LocOffsets docLoc, VALUE arg, bool optional) {
    auto loc = rbsLoc(docLoc, rb_funcall(arg, rb_intern("location"), 0));
    auto name = rb_funcall(arg, rb_intern("name"), 0);
    auto type = rb_funcall(arg, rb_intern("type"), 0);

    return RBSArg{loc, arg, name, type, optional};
}

void collectArgs(core::LocOffsets docLoc, VALUE field, std::vector<RBSArg> &args, bool optional) {
    for (int i = 0; i < RARRAY_LEN(field); i++) {
        VALUE argValue = rb_ary_entry(field, i);
        args.emplace_back(rbsArg(docLoc, argValue, optional));
    }
}

void collectKeywords(core::LocOffsets docLoc, VALUE field, std::vector<RBSArg> &args, bool optional) {
    VALUE keys = rb_funcall(field, rb_intern("keys"), 0);
    long size = RARRAY_LEN(keys);

    for (long i = 0; i < size; i++) {
        VALUE key = rb_ary_entry(keys, i);
        VALUE value = rb_hash_aref(field, key);

        auto arg = rbsArg(docLoc, value, optional);
        arg.name = key;
        args.emplace_back(arg);
    }
}

} // namespace

sorbet::ast::ExpressionPtr MethodTypeTranslator::toRBI(core::MutableContext ctx, core::LocOffsets docLoc,
                                                       sorbet::ast::MethodDef *methodDef, VALUE methodType) {
    // TODO raise error if methodType is not a MethodType
    // std::cout << "METHOD DEF: " << methodDef->showRaw(ctx) << std::endl;
    // std::cout << rb_obj_classname(methodType) << std::endl;
    rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);

    Send::ARGS_store sigArgs;

    std::vector<RBSArg> args;

    collectArgs(docLoc, rb_funcall(functionType, rb_intern("required_positionals"), 0), args, false);
    collectArgs(docLoc, rb_funcall(functionType, rb_intern("optional_positionals"), 0), args, false);

    VALUE restPositionals = rb_funcall(functionType, rb_intern("rest_positionals"), 0);
    if (restPositionals != Qnil) {
        args.emplace_back(rbsArg(docLoc, restPositionals, false));
    }

    collectArgs(docLoc, rb_funcall(functionType, rb_intern("trailing_positionals"), 0), args, false);

    collectKeywords(docLoc, rb_funcall(functionType, rb_intern("required_keywords"), 0), args, false);
    collectKeywords(docLoc, rb_funcall(functionType, rb_intern("optional_keywords"), 0), args, false);

    VALUE restKeywords = rb_funcall(functionType, rb_intern("rest_keywords"), 0);
    if (restKeywords != Qnil) {
        args.emplace_back(rbsArg(docLoc, restKeywords, false));
    }

    VALUE block = rb_funcall(methodType, rb_intern("block"), 0);
    if (block != Qnil) {
        // TODO: RBS doesn't have location on blocks?
        auto loc = docLoc;
        auto name = Qnil;
        auto type = rb_funcall(block, rb_intern("type"), 0);
        auto optional = !RTEST(rb_funcall(block, rb_intern("required"), 0));
        args.emplace_back(RBSArg{loc, block, name, type, optional});
    }

    for (int i = 0; i < args.size(); i++) {
        auto &arg = args[i];
        // std::cout << "PARAM: " << arg.loc.showRaw(ctx) << std::endl;
        // rb_p(arg.value);

        core::NameRef name;
        auto nameValue = arg.name;

        if (nameValue != Qnil) {
            // The RBS arg is named in the signature, so we use that name.
            auto nameString = rb_funcall(nameValue, rb_intern("to_s"), 0);
            const char *nameStr = StringValueCStr(nameString);
            name = ctx.state.enterNameUTF8(nameStr);
        } else {
            // The RBS arg is not named in the signature, so we look at the method definition
            auto &methodArg = methodDef->args[i];
            name = expressionName(ctx, &methodArg);
        }

        auto type = TypeTranslator::toRBI(ctx, arg.type, arg.loc);
        if (arg.optional) {
            type = ast::MK::Nilable(arg.loc, std::move(type));
        }

        sigArgs.emplace_back(ast::MK::Symbol(arg.loc, name));
        sigArgs.emplace_back(std::move(type));
    }

    VALUE returnValue = rb_funcall(functionType, rb_intern("return_type"), 0);
    if (strcmp(rb_obj_classname(returnValue), "RBS::Types::Bases::Void") == 0) {
        return ast::MK::SigVoid(docLoc, std::move(sigArgs));
    }

    auto returnType = TypeTranslator::toRBI(ctx, returnValue, methodDef->loc);
    return ast::MK::Sig(docLoc, std::move(sigArgs), std::move(returnType));
}

} // namespace sorbet::rbs
