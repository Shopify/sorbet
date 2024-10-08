#include "MethodTypeTranslator.h"
#include "TypeTranslator.h"
#include "absl/strings/escaping.h"
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

std::pair<core::NameRef, core::LocOffsets> getName(core::MutableContext ctx, ast::ExpressionPtr &name) {
    core::LocOffsets loc;
    core::NameRef res;
    if (auto *lit = ast::cast_tree<ast::Literal>(name)) {
        if (lit->isSymbol()) {
            res = lit->asSymbol();
            loc = lit->loc;
            ENFORCE(ctx.locAt(loc).exists());
            ENFORCE(ctx.locAt(loc).source(ctx).value().size() > 1 && ctx.locAt(loc).source(ctx).value()[0] == ':');
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos()};
        } else if (lit->isString()) {
            core::NameRef nameRef = lit->asString();
            auto shortName = nameRef.shortName(ctx);
            bool validAttr = (isalpha(shortName.front()) || shortName.front() == '_') &&
                             absl::c_all_of(shortName, [](char c) { return isalnum(c) || c == '_'; });
            if (validAttr) {
                res = nameRef;
            } else {
                if (auto e = ctx.beginError(name.loc(), core::errors::Rewriter::BadAttrArg)) {
                    e.setHeader("Bad attribute name \"{}\"", absl::CEscape(shortName));
                }
                res = core::Names::empty();
            }
            loc = lit->loc;
            DEBUG_ONLY({
                auto l = ctx.locAt(loc);
                ENFORCE(l.exists());
                auto source = l.source(ctx).value();
                ENFORCE(source.size() > 2);
                ENFORCE(source[0] == '"' || source[0] == '\'');
                auto lastChar = source[source.size() - 1];
                ENFORCE(lastChar == '"' || lastChar == '\'');
            });
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos() - 1};
        }
    }
    if (!res.exists()) {
        if (auto e = ctx.beginError(name.loc(), core::errors::Rewriter::BadAttrArg)) {
            e.setHeader("arg must be a Symbol or String");
        }
    }
    return std::make_pair(res, loc);
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

sorbet::ast::ExpressionPtr MethodTypeTranslator::methodSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                                 sorbet::ast::MethodDef *methodDef, VALUE methodType,
                                                                 std::vector<RBSAnnotation> annotations) {
    // TODO raise error if methodType is not a MethodType
    // std::cout << "METHOD DEF: " << methodDef->showRaw(ctx) << std::endl;
    // std::cout << rb_obj_classname(methodType) << std::endl;
    // rb_p(methodType);

    VALUE functionType = rb_funcall(methodType, rb_intern("type"), 0);

    Send::ARGS_store sigArgs;

    std::vector<RBSArg> args;

    collectArgs(docLoc, rb_funcall(functionType, rb_intern("required_positionals"), 0), args, false);

    VALUE optionalPositionals = rb_funcall(functionType, rb_intern("optional_positionals"), 0);
    if (optionalPositionals != Qfalse) {
        collectArgs(docLoc, optionalPositionals, args, true);
    }
    VALUE restPositionals = rb_funcall(functionType, rb_intern("rest_positionals"), 0);
    if (restPositionals != Qnil) {
        args.emplace_back(rbsArg(docLoc, restPositionals, false));
    }

    VALUE trailingPositionals = rb_funcall(functionType, rb_intern("trailing_positionals"), 0);
    if (trailingPositionals != Qfalse) {
        collectArgs(docLoc, trailingPositionals, args, false);
    }

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
        auto type = block;
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
            name = expressionName(ctx, &methodArg);
        }

        auto type = TypeTranslator::toRBI(ctx, arg.type, arg.loc);
        if (arg.optional) {
            type = ast::MK::Nilable(arg.loc, std::move(type));
        }

        sigArgs.emplace_back(ast::MK::Symbol(arg.loc, name));
        sigArgs.emplace_back(std::move(type));
    }

    auto sigBuilder = ast::MK::Self(docLoc);

    for (auto &annotation : annotations) {
        if (annotation.string == "@abstract") {
            sigBuilder = ast::MK::Send0(annotation.loc, std::move(sigBuilder), core::Names::abstract(), annotation.loc);
        } else if (annotation.string == "@override") {
            sigBuilder =
                ast::MK::Send0(annotation.loc, std::move(sigBuilder), core::Names::override_(), annotation.loc);
        }
    }

    ENFORCE(sigArgs.size() % 2 == 0, "Sig params must be arg name/type pairs");

    if (sigArgs.size() > 0) {
        sigBuilder = ast::MK::Send(docLoc, std::move(sigBuilder), core::Names::params(), docLoc, 0, std::move(sigArgs));
    }

    VALUE returnValue = rb_funcall(functionType, rb_intern("return_type"), 0);
    if (strcmp(rb_obj_classname(returnValue), "RBS::Types::Bases::Void") == 0) {
        sigBuilder = ast::MK::Send0(docLoc, std::move(sigBuilder), core::Names::void_(), docLoc);
    } else {
        auto returnType = TypeTranslator::toRBI(ctx, returnValue, methodDef->loc);
        sigBuilder =
            ast::MK::Send1(docLoc, std::move(sigBuilder), core::Names::returns(), docLoc, std::move(returnType));
    }

    auto sig =
        ast::MK::Send1(docLoc, ast::MK::Constant(docLoc, core::Symbols::Sorbet_Private_Static()), core::Names::sig(),
                       docLoc, ast::MK::Constant(docLoc, core::Symbols::T_Sig_WithoutRuntime()));
    auto sigSend = ast::cast_tree<ast::Send>(sig);
    sigSend->setBlock(ast::MK::Block0(docLoc, std::move(sigBuilder)));
    sigSend->flags.isRewriterSynthesized = true;

    return sig;
}

sorbet::ast::ExpressionPtr MethodTypeTranslator::attrSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                               sorbet::ast::Send *send, VALUE attrType,
                                                               std::vector<RBSAnnotation> annotations) {
    // TODO raise error if attr is not a Type
    // std::cout << "METHOD DEF: " << methodDef->showRaw(ctx) << std::endl;
    // std::cout << rb_obj_classname(methodType) << std::endl;

    auto sigBuilder = ast::MK::Self(docLoc);

    if (send->fun == core::Names::attrWriter()) {
        ENFORCE(send->numPosArgs() >= 1);
        auto &arg = send->getPosArg(0);
        auto name = getName(ctx, arg);

        Send::ARGS_store sigArgs;
        sigArgs.emplace_back(ast::MK::Symbol(name.second, name.first));
        sigArgs.emplace_back(TypeTranslator::toRBI(ctx, attrType, docLoc));
        sigBuilder = ast::MK::Send(docLoc, std::move(sigBuilder), core::Names::params(), docLoc, 0, std::move(sigArgs));
    }

    auto returnType = TypeTranslator::toRBI(ctx, attrType, docLoc);
    sigBuilder = ast::MK::Send1(docLoc, std::move(sigBuilder), core::Names::returns(), docLoc, std::move(returnType));

    auto sig =
        ast::MK::Send1(docLoc, ast::MK::Constant(docLoc, core::Symbols::Sorbet_Private_Static()), core::Names::sig(),
                       docLoc, ast::MK::Constant(docLoc, core::Symbols::T_Sig_WithoutRuntime()));
    auto sigSend = ast::cast_tree<ast::Send>(sig);
    sigSend->setBlock(ast::MK::Block0(docLoc, std::move(sigBuilder)));
    sigSend->flags.isRewriterSynthesized = true;

    return sig;
}

} // namespace sorbet::rbs
