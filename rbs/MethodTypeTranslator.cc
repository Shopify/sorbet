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
    // rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);

    Send::ARGS_store sigArgs;

    std::vector<RBSArg> args;

    collectArgs(docLoc, rb_funcall(functionType, rb_intern("required_positionals"), 0), args, false);
    collectArgs(docLoc, rb_funcall(functionType, rb_intern("optional_positionals"), 0), args, true);

    VALUE restPositionals = rb_funcall(functionType, rb_intern("rest_positionals"), 0);
    if (restPositionals != Qnil) {
        args.emplace_back(rbsArg(docLoc, restPositionals, false));
    }

    collectArgs(docLoc, rb_funcall(functionType, rb_intern("trailing_positionals"), 0), args, false);

    collectKeywords(docLoc, rb_funcall(functionType, rb_intern("required_keywords"), 0), args, false);
    collectKeywords(docLoc, rb_funcall(functionType, rb_intern("optional_keywords"), 0), args, true);

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
        args.emplace_back(RBSArg{loc, block, name, type, false});
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
            // std::cout << "METHOD ARG: " << methodArg.showRaw(ctx) << std::endl;
            name = expressionName(ctx, &methodArg);
        }

        auto type = TypeTranslator::toRBI(ctx, arg.type, arg.loc);
        if (arg.optional) {
            type = ast::MK::Nilable(arg.loc, std::move(type));
        }

        sigArgs.emplace_back(ast::MK::Symbol(arg.loc, name));
        sigArgs.emplace_back(std::move(type));
    }

    // VALUE requiredKeywordsValue = rb_funcall(functionType, rb_intern("required_keywords"), 0);
    // VALUE optionalKeywordsValue = rb_funcall(functionType, rb_intern("optional_keywords"), 0);
    // VALUE restKeywordsValue = rb_funcall(functionType, rb_intern("rest_keywords"), 0);
    // VALUE blockValue = rb_funcall(methodType, rb_intern("block"), 0);

    // Iterate over positionals
    // find matching positional in positionals array

    // Iterate over keywords
    // find matching keyword in keywords hash

    // Add block

    // for (int i = 0; i < methodDef->args.size(); i++) {
    //     auto &methodArg = methodDef->args[i];

    //     VALUE argValue;
    //     core::NameRef argName;
    //     bool asNilable = false;
    //     // std::cout << "ARG: " << methodArg.showRaw(ctx) << std::endl;

    //     typecase(
    //         methodArg,
    //         [&](const ast::OptionalArg &p) {
    //             typecase(
    //                 p.expr,
    //                 [&](const ast::KeywordArg &p) {
    //                     auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
    //                     if (argIdent != nullptr) {
    //                         argName = argIdent->name;
    //                         VALUE key = ID2SYM(rb_intern(argName.show(ctx).c_str()));
    //                         argValue = rb_hash_aref(optionalKeywordsValue, key);
    //                         asNilable = true;
    //                     }
    //                 },
    //                 [&](const ast::ExpressionPtr &p) {
    //                     std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
    //                 });
    //         },
    //         [&](const ast::RestArg &p) {
    //             typecase(
    //                 p.expr,
    //                 [&](const ast::KeywordArg &p) {
    //                     auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
    //                     if (argIdent != nullptr) {
    //                         argName = argIdent->name;
    //                     }
    //                     argValue = restKeywordsValue;
    //                 },
    //                 [&](const ast::ExpressionPtr &p) {
    //                     std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl;
    //                 });
    //         },
    //         // TODO: Access the hash
    //         [&](const ast::KeywordArg &p) {
    //             auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
    //             if (argIdent != nullptr) {
    //                 argName = argIdent->name;
    //                 VALUE key = ID2SYM(rb_intern(argName.show(ctx).c_str()));
    //                 argValue = rb_hash_aref(requiredKeywordsValue, key);
    //             }
    //         },
    //         [&](const ast::BlockArg &p) {
    //             auto argIdent = ast::cast_tree<ast::UnresolvedIdent>(p.expr);
    //             if (argIdent != nullptr) {
    //                 argName = argIdent->name;
    //             }
    //             argValue = blockValue;
    //         },
    //         [&](const ast::ExpressionPtr &p) { std::cout << "UNKNOWN EXPRESSION " << p.showRaw(ctx) << std::endl; });

    //     if (!argName.exists()) {
    //         std::cout << "MISSING ARG NAME: " << methodArg.showRaw(ctx) << std::endl;
    //         // TODO raise error
    //         continue;
    //     } else {
    //         // std::cout << "ARG NAME: " << argName.show(ctx) << std::endl;
    //     }

    //     if (argValue == Qnil) {
    //         // if (auto e = ctx.beginError(docLoc, core::errors::Rewriter::RBSError)) {
    //         //     e.addErrorNote("Malformed `sig`. Type not specified for argument `{}`", argName.show(ctx));
    //         // }
    //         continue;
    //     }

    //     auto argTypeValue = rb_funcall(argValue, rb_intern("type"), 0);
    //     auto argType = TypeTranslator::toRBI(ctx, argTypeValue, methodArg.loc());
    //     if (asNilable) {
    //         argType = ast::MK::Nilable(methodArg.loc(), std::move(argType));
    //     }

    //     VALUE rbsLoc = rb_funcall(argValue, rb_intern("location"), 0);
    //     VALUE rbsStartColumnValue = rb_funcall(rbsLoc, rb_intern("start_column"), 0);
    //     int rbsStartColumn = NUM2INT(rbsStartColumnValue);
    //     VALUE rbsEndColumnValue = rb_funcall(rbsLoc, rb_intern("end_column"), 0);
    //     int rbsEndColumn = NUM2INT(rbsEndColumnValue);
    //     core::LocOffsets rbiOffsets{docLoc.beginLoc + rbsStartColumn + 2, docLoc.beginLoc + rbsEndColumn + 2};
    //     // std::cout << "DOC LOC: " << docLoc.showRaw(ctx) << std::endl;
    //     // std::cout << "RBI OFFSETS: " << rbiOffsets.showRaw(ctx) << std::endl;

    //     sigArgs.emplace_back(ast::MK::Symbol(rbiOffsets, argName));
    //     sigArgs.emplace_back(std::move(argType));
    // }

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
