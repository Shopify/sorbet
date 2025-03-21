#include "rbs/MethodTypeTranslator.h"

#include "absl/algorithm/container.h"
#include "absl/strings/escaping.h"
#include "common/typecase.h"
#include "core/GlobalState.h"
#include "core/errors/internal.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "rbs/TypeTranslator.h"
#include "rewriter/util/Util.h"

using namespace std;

namespace sorbet::rbs {

namespace {

unique_ptr<parser::Node> handleAnnotationsNode(unique_ptr<parser::Node> sigBuilder,
                                               const vector<Comment> &annotations) {
    for (auto &annotation : annotations) {
        if (annotation.string == "final") {
            // no-op, `final` is handled in the `sig()` call later
        } else if (annotation.string == "abstract") {
            sigBuilder = parser::MK::Send0(annotation.loc, move(sigBuilder), core::Names::abstract(), annotation.loc);
        } else if (annotation.string == "overridable") {
            sigBuilder =
                parser::MK::Send0(annotation.loc, move(sigBuilder), core::Names::overridable(), annotation.loc);
        } else if (annotation.string == "override") {
            sigBuilder = parser::MK::Send0(annotation.loc, move(sigBuilder), core::Names::override_(), annotation.loc);
        } else if (annotation.string == "override(allow_incompatible: true)") {
            auto pairs = parser::NodeVec();
            auto key = parser::MK::Symbol(annotation.loc, core::Names::allowIncompatible());
            auto value = parser::MK::True(annotation.loc);
            pairs.emplace_back(make_unique<parser::Pair>(annotation.loc, move(key), move(value)));
            auto hash = parser::MK::Hash(annotation.loc, true, move(pairs));

            auto args = parser::NodeVec();
            args.emplace_back(move(hash));

            sigBuilder = parser::MK::Send(annotation.loc, move(sigBuilder), core::Names::override_(), annotation.loc,
                                          move(args));
        }
    }

    return sigBuilder;
}

core::NameRef nodeName(parser::Node &node) {
    core::NameRef name;

    typecase(
        &node, [&](const parser::Arg *a) { name = a->name; }, [&](const parser::Restarg *a) { name = a->name; },
        [&](const parser::Kwarg *a) { name = a->name; }, [&](const parser::Blockarg *a) { name = a->name; },
        [&](const parser::Kwoptarg *a) { name = a->name; }, [&](const parser::Optarg *a) { name = a->name; },
        [&](const parser::Kwrestarg *a) { name = a->name; }, [&](const parser::Shadowarg *a) { name = a->name; },
        [&](const parser::Symbol *s) { name = s->val; },
        [&](const parser::Node *other) { Exception::raise("Unexpected expression type: {}", node.nodeName()); });

    return name;
}

parser::Args *getMethodArgs(parser::Node &node) {
    parser::Node *args;

    typecase(
        &node, [&](const parser::DefMethod *defMethod) { args = defMethod->args.get(); },
        [&](const parser::DefS *defS) { args = defS->args.get(); },
        [&](const parser::Node *other) { Exception::raise("Unexpected expression type: {}", node.nodeName()); });

    return parser::cast_node<parser::Args>(args);
}

struct RBSArg {
    core::LocOffsets loc;
    rbs_ast_symbol_t *name;
    rbs_node_t *type;
};

void collectArgs(core::MutableContext ctx, core::LocOffsets docLoc, rbs_node_list_t *field, vector<RBSArg> &args) {
    if (field == nullptr || field->length == 0) {
        return;
    }

    for (rbs_node_list_node_t *list_node = field->head; list_node != nullptr; list_node = list_node->next) {
        ENFORCE(list_node->node->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in function parameter list, expected `{}`",
                rbs_node_type_name(list_node->node), "FunctionParam");

        auto loc = locFromRange(docLoc, list_node->node->location->rg);
        auto node = (rbs_types_function_param_t *)list_node->node;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }
}

void collectKeywords(core::MutableContext ctx, core::LocOffsets docLoc, rbs_hash_t *field, vector<RBSArg> &args) {
    if (field == nullptr) {
        return;
    }

    for (rbs_hash_node_t *hash_node = field->head; hash_node != nullptr; hash_node = hash_node->next) {
        ENFORCE(hash_node->key->type == RBS_AST_SYMBOL,
                "Unexpected node type `{}` in keyword argument name, expected `{}`", rbs_node_type_name(hash_node->key),
                "Symbol");

        ENFORCE(hash_node->value->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in keyword argument value, expected `{}`",
                rbs_node_type_name(hash_node->value), "FunctionParam");

        auto loc = locFromRange(docLoc, hash_node->key->location->rg);
        rbs_ast_symbol_t *keyNode = (rbs_ast_symbol_t *)hash_node->key;
        rbs_types_function_param_t *valueNode = (rbs_types_function_param_t *)hash_node->value;
        auto arg = RBSArg{loc, keyNode, valueNode->type};
        args.emplace_back(arg);
    }
}

} // namespace

unique_ptr<parser::Node> MethodTypeTranslator::methodSignature(core::MutableContext ctx, parser::Node *methodDef,
                                                               core::LocOffsets commentLoc, const MethodType methodType,
                                                               const vector<Comment> &annotations) {
    const auto &node = *methodType.node;

    if (node.type->type != RBS_TYPES_FUNCTION) {
        auto errLoc = locFromRange(methodType.loc, node.type->location->rg);
        if (auto e = ctx.beginError(errLoc, core::errors::Rewriter::RBSUnsupported)) {
            e.setHeader("Unexpected node type `{}` in method signature, expected `{}`", rbs_node_type_name(node.type),
                        "Function");
        }

        return nullptr;
    }
    auto *functionType = (rbs_types_function_t *)node.type;

    // Collect type parameters

    vector<pair<core::LocOffsets, core::NameRef>> typeParams;
    for (rbs_node_list_node_t *list_node = node.type_params->head; list_node != nullptr; list_node = list_node->next) {
        auto loc = locFromRange(methodType.loc, list_node->node->location->rg);

        ENFORCE(list_node->node->type == RBS_AST_TYPEPARAM,
                "Unexpected node type `{}` in type parameter list, expected `{}`", rbs_node_type_name(list_node->node),
                "TypeParam");

        auto node = (rbs_ast_typeparam_t *)list_node->node;
        rbs_constant_t *constant = rbs_constant_pool_id_to_constant(fake_constant_pool, node->name->constant_id);
        string_view str(constant->start, constant->length);
        typeParams.emplace_back(loc, ctx.state.enterNameUTF8(str));
    }

    // Collect positionals

    vector<RBSArg> args;

    collectArgs(ctx, methodType.loc, functionType->required_positionals, args);
    collectArgs(ctx, methodType.loc, functionType->optional_positionals, args);

    rbs_node_t *restPositionals = functionType->rest_positionals;
    if (restPositionals) {
        ENFORCE(restPositionals->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest positional argument, expected `{}`",
                rbs_node_type_name(restPositionals), "FunctionParam");

        auto loc = locFromRange(methodType.loc, restPositionals->location->rg);
        auto node = (rbs_types_function_param_t *)restPositionals;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }

    collectArgs(ctx, methodType.loc, functionType->trailing_positionals, args);

    // Collect keywords

    collectKeywords(ctx, methodType.loc, functionType->required_keywords, args);
    collectKeywords(ctx, methodType.loc, functionType->optional_keywords, args);

    rbs_node_t *restKeywords = functionType->rest_keywords;
    if (restKeywords) {
        ENFORCE(restKeywords->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest keyword argument, expected `{}`", rbs_node_type_name(restKeywords),
                "FunctionParam");

        auto loc = locFromRange(methodType.loc, restKeywords->location->rg);
        auto node = (rbs_types_function_param_t *)restKeywords;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }

    // Collect block

    auto *block = node.block;
    if (block) {
        // TODO: RBS doesn't have location on blocks yet
        auto arg = RBSArg{methodType.loc, nullptr, (rbs_node_t *)block};
        args.emplace_back(arg);
    }

    auto sigParams = parser::NodeVec();
    sigParams.reserve(args.size());

    auto methodArgs = getMethodArgs(*methodDef);
    for (int i = 0; i < args.size(); i++) {
        auto &arg = args[i];
        core::NameRef name;
        auto nameSymbol = arg.name;

        if (nameSymbol) {
            // The RBS arg is named in the signature, so we use the explicit name used
            rbs_constant_t *nameConstant =
                rbs_constant_pool_id_to_constant(fake_constant_pool, nameSymbol->constant_id);
            string_view nameStr(nameConstant->start, nameConstant->length);
            name = ctx.state.enterNameUTF8(nameStr);
        } else {
            if (!methodArgs || i >= methodArgs->args.size()) {
                if (auto e = ctx.beginError(methodType.loc, core::errors::Rewriter::RBSParameterMismatch)) {
                    e.setHeader("RBS signature has more parameters than in the method definition");
                }

                return nullptr;
            }

            // The RBS arg is not named in the signature, so we get it from the method definition
            name = nodeName(*methodArgs->args[i].get());
        }

        auto type = TypeTranslator::toParserNode(ctx, typeParams, arg.type, methodType.loc);
        auto pair = make_unique<parser::Pair>(arg.loc, parser::MK::Symbol(arg.loc, name), move(type));
        sigParams.emplace_back(move(pair));
    }

    // Build the sig

    auto sigBuilder = parser::MK::Self(commentLoc);
    sigBuilder = handleAnnotationsNode(std::move(sigBuilder), annotations);

    if (typeParams.size() > 0) {
        auto typeParamsVector = parser::NodeVec();
        typeParamsVector.reserve(typeParams.size());

        for (auto &param : typeParams) {
            typeParamsVector.emplace_back(parser::MK::Symbol(param.first, param.second));
        }
        sigBuilder = parser::MK::Send(commentLoc, move(sigBuilder), core::Names::typeParameters(), commentLoc,
                                      move(typeParamsVector));
    }

    if (sigParams.size() > 0) {
        auto hash = parser::MK::Hash(methodType.loc, true, move(sigParams));
        auto args = parser::NodeVec();
        args.emplace_back(move(hash));
        sigBuilder = parser::MK::Send(commentLoc, move(sigBuilder), core::Names::params(), commentLoc, move(args));
    }

    rbs_node_t *returnValue = functionType->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        auto loc = locFromRange(methodType.loc, returnValue->location->rg);
        sigBuilder = parser::MK::Send0(loc, move(sigBuilder), core::Names::void_(), loc);
    } else {
        auto returnType = TypeTranslator::toParserNode(ctx, typeParams, returnValue, methodType.loc);
        sigBuilder =
            parser::MK::Send1(commentLoc, move(sigBuilder), core::Names::returns(), commentLoc, move(returnType));
    }

    auto sigArgs = parser::NodeVec();
    auto t = parser::MK::T(commentLoc);
    auto t_sig = parser::MK::Const(commentLoc, move(t), core::Names::Constants::Sig());
    auto t_sig_withoutRuntime = parser::MK::Const(commentLoc, move(t_sig), core::Names::Constants::WithoutRuntime());
    sigArgs.emplace_back(move(t_sig_withoutRuntime));

    bool isFinal = absl::c_any_of(annotations, [](const Comment &annotation) { return annotation.string == "final"; });

    if (isFinal) {
        sigArgs.emplace_back(parser::MK::Symbol(methodType.loc, core::Names::final_()));
    }

    auto sorbet = parser::MK::Const(commentLoc, parser::MK::Cbase(commentLoc), core::Names::Constants::Sorbet());
    auto sorbet_private = parser::MK::Const(commentLoc, move(sorbet), core::Names::Constants::Private());
    auto sorbet_private_static = parser::MK::Const(commentLoc, move(sorbet_private), core::Names::Constants::Static());
    auto sig = parser::MK::Send(commentLoc, move(sorbet_private_static), core::Names::sig(), commentLoc, move(sigArgs));

    return make_unique<parser::Block>(commentLoc, move(sig), nullptr, move(sigBuilder));
}

unique_ptr<parser::Node> MethodTypeTranslator::attrSignature(core::MutableContext ctx, const parser::Send *send,
                                                             core::LocOffsets commentLoc, const Type attrType,
                                                             const vector<Comment> &annotations) {
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>();
    auto sigBuilder = parser::MK::Self(commentLoc);
    sigBuilder = handleAnnotationsNode(std::move(sigBuilder), annotations);

    if (send->args.size() == 0) {
        if (auto e = ctx.beginError(send->loc, core::errors::Rewriter::RBSUnsupported)) {
            e.setHeader("RBS signatures do not support accessor without arguments");
        }

        return nullptr;
    }

    if (send->method == core::Names::attrWriter()) {
        if (send->args.size() > 1) {
            if (auto e = ctx.beginError(send->loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for attr_writer do not support multiple arguments");
            }

            return nullptr;
        }

        // For attr writer, we need to add the param to the sig
        auto name = nodeName(*send->args[0].get());
        std::cerr << "name: " << name.show(ctx) << " kind: " << std::endl;
        auto pairs = parser::NodeVec();
        pairs.emplace_back(make_unique<parser::Pair>(
            send->args[0]->loc, parser::MK::Symbol(send->args[0]->loc, name),
            TypeTranslator::toParserNode(ctx, typeParams, attrType.node.get(), attrType.loc)));
        auto hash = parser::MK::Hash(send->loc, true, move(pairs));
        auto sigArgs = parser::NodeVec();
        sigArgs.emplace_back(move(hash));
        sigBuilder = parser::MK::Send(commentLoc, move(sigBuilder), core::Names::params(), commentLoc, move(sigArgs));
    }

    sigBuilder = parser::MK::Send1(commentLoc, move(sigBuilder), core::Names::returns(), commentLoc,
                                   TypeTranslator::toParserNode(ctx, typeParams, attrType.node.get(), attrType.loc));

    auto sigArgs = parser::NodeVec();
    auto t = parser::MK::T(commentLoc);
    auto t_sig = parser::MK::Const(commentLoc, move(t), core::Names::Constants::Sig());
    auto t_sig_withoutRuntime = parser::MK::Const(commentLoc, move(t_sig), core::Names::Constants::WithoutRuntime());
    sigArgs.emplace_back(move(t_sig_withoutRuntime));

    auto sorbet = parser::MK::Const(commentLoc, parser::MK::Cbase(commentLoc), core::Names::Constants::Sorbet());
    auto sorbet_private = parser::MK::Const(commentLoc, move(sorbet), core::Names::Constants::Private());
    auto sorbet_private_static = parser::MK::Const(commentLoc, move(sorbet_private), core::Names::Constants::Static());
    auto sig = parser::MK::Send(commentLoc, move(sorbet_private_static), core::Names::sig(), commentLoc, move(sigArgs));

    return make_unique<parser::Block>(commentLoc, move(sig), nullptr, move(sigBuilder));
}

} // namespace sorbet::rbs
