#include "rbs/MethodTypeToParserNode.h"

#include "absl/algorithm/container.h"
#include "absl/strings/escaping.h"
#include "common/typecase.h"
#include "core/GlobalState.h"
#include "core/errors/internal.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "rbs/TypeToParserNode.h"
#include "rewriter/util/Util.h"

using namespace std;

namespace sorbet::rbs {

namespace {

struct RBSArg {
    core::LocOffsets loc;
    rbs_ast_symbol_t *name;
    rbs_node_t *type;
};

unique_ptr<parser::Node> handleAnnotations(core::MutableContext ctx, unique_ptr<parser::Node> sigBuilder,
                                           const vector<Comment> &annotations) {
    for (auto &annotation : annotations) {
        if (annotation.string == "final") {
            // no-op, `final` is handled in the `sig()` call later
        } else if (annotation.string == "abstract") {
            if (auto e = ctx.beginError(annotation.typeLoc, core::errors::Rewriter::RBSAbstractMisformatted)) {
                e.setHeader(
                    "RBS signatures for abstract methods must be formatted as `# @abstract: def name: () -> void`");
                continue;
            }
        } else if (annotation.string == "overridable") {
            sigBuilder =
                parser::MK::Send0(annotation.typeLoc, move(sigBuilder), core::Names::overridable(), annotation.typeLoc);
        } else if (annotation.string == "override") {
            sigBuilder =
                parser::MK::Send0(annotation.typeLoc, move(sigBuilder), core::Names::override_(), annotation.typeLoc);
        } else if (annotation.string == "override(allow_incompatible: true)") {
            auto pairs = parser::NodeVec();
            auto key = parser::MK::Symbol(annotation.typeLoc, core::Names::allowIncompatible());
            auto value = parser::MK::True(annotation.typeLoc);
            pairs.emplace_back(make_unique<parser::Pair>(annotation.typeLoc, move(key), move(value)));
            auto hash = parser::MK::Hash(annotation.typeLoc, true, move(pairs));

            auto args = parser::NodeVec();
            args.emplace_back(move(hash));

            sigBuilder = parser::MK::Send(annotation.typeLoc, move(sigBuilder), core::Names::override_(),
                                          annotation.typeLoc, move(args));
        }
    }

    return sigBuilder;
}

core::NameRef nodeName(const parser::Node *node) {
    core::NameRef name;

    typecase(
        node, [&](const parser::Arg *a) { name = a->name; }, [&](const parser::Restarg *a) { name = a->name; },
        [&](const parser::Kwarg *a) { name = a->name; }, [&](const parser::Blockarg *a) { name = a->name; },
        [&](const parser::Kwoptarg *a) { name = a->name; }, [&](const parser::Optarg *a) { name = a->name; },
        [&](const parser::Kwrestarg *a) { name = a->name; }, [&](const parser::Shadowarg *a) { name = a->name; },
        [&](const parser::Symbol *s) { name = s->val; },
        [&](const parser::Node *other) {
            Exception::raise("Unexpected expression type: {}", ((parser::Node *)node)->nodeName());
        });

    return name;
}

parser::Args *getMethodArgs(const parser::Node *node) {
    parser::Node *args;

    typecase(
        node, [&](const parser::DefMethod *defMethod) { args = defMethod->args.get(); },
        [&](const parser::DefS *defS) { args = defS->args.get(); },
        [&](const parser::Node *other) {
            Exception::raise("Unexpected expression type: {}", ((parser::Node *)node)->nodeName());
        });

    return parser::cast_node<parser::Args>(args);
}

void collectArgs(const RBSDeclaration &declaration, rbs_node_list_t *field, vector<RBSArg> &args) {
    if (field == nullptr || field->length == 0) {
        return;
    }

    for (rbs_node_list_node_t *list_node = field->head; list_node != nullptr; list_node = list_node->next) {
        ENFORCE(list_node->node->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in function parameter list, expected `{}`",
                rbs_node_type_name(list_node->node), "FunctionParam");

        auto range = list_node->node->location->rg;

        // If the arg is named, we need to adjust the location to point to the name
        auto nameRange = list_node->node->location->children->entries[0].rg;
        if (nameRange.start != -1 && nameRange.end != -1) {
            range.start.char_pos = nameRange.start;
            range.end.char_pos = nameRange.end;
        }

        auto loc = declaration.typeLocFromRange(range);
        auto node = (rbs_types_function_param_t *)list_node->node;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }
}

void collectKeywords(const RBSDeclaration &declaration, rbs_hash_t *field, vector<RBSArg> &args) {
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

        auto loc = declaration.typeLocFromRange(hash_node->key->location->rg);
        rbs_ast_symbol_t *keyNode = (rbs_ast_symbol_t *)hash_node->key;
        rbs_types_function_param_t *valueNode = (rbs_types_function_param_t *)hash_node->value;
        auto arg = RBSArg{loc, keyNode, valueNode->type};
        args.emplace_back(arg);
    }
}

unique_ptr<parser::Node> assembleTSigWithoutRuntime(const core::LocOffsets &loc) {
    auto t = parser::MK::T(loc);
    auto t_sig = parser::MK::Const(loc, move(t), core::Names::Constants::Sig());
    auto t_sig_withoutRuntime = parser::MK::Const(loc, move(t_sig), core::Names::Constants::WithoutRuntime());
    return t_sig_withoutRuntime;
}

} // namespace

unique_ptr<parser::Node> MethodTypeToParserNode::methodSignature(const parser::Node *methodDef,
                                                                 const rbs_method_type_t *methodType,
                                                                 const RBSDeclaration &declaration,
                                                                 const vector<Comment> &annotations, bool abstract) {
    const auto &node = *methodType;
    // Method signatures can have multiple lines, so we need
    // - full type location
    // - first line type location
    // - token specific location
    // for different parts of the signature
    //
    // For example,
    // - The whole signature block needs to be mapped to the full type location
    // - The `sig` call is always mapped to just the first line of the signature
    // - The return value needs to has a calculated location based on the token range
    auto fullTypeLoc = declaration.fullTypeLoc();
    auto firstLineTypeLoc = declaration.firstLineTypeLoc();
    auto commentLoc = declaration.commentLoc();

    if (node.type->type != RBS_TYPES_FUNCTION) {
        auto errLoc = declaration.typeLocFromRange(node.type->location->rg);
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
        auto loc = declaration.typeLocFromRange(list_node->node->location->rg);

        ENFORCE(list_node->node->type == RBS_AST_TYPE_PARAM,
                "Unexpected node type `{}` in type parameter list, expected `{}`", rbs_node_type_name(list_node->node),
                "TypeParam");

        auto node = (rbs_ast_type_param_t *)list_node->node;
        auto str = parser.resolveConstant(node->name);
        typeParams.emplace_back(loc, ctx.state.enterNameUTF8(str));
    }

    // Collect positionals

    vector<RBSArg> args;

    collectArgs(declaration, functionType->required_positionals, args);
    collectArgs(declaration, functionType->optional_positionals, args);

    rbs_node_t *restPositionals = functionType->rest_positionals;
    if (restPositionals) {
        ENFORCE(restPositionals->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest positional argument, expected `{}`",
                rbs_node_type_name(restPositionals), "FunctionParam");

        auto loc = declaration.typeLocFromRange(restPositionals->location->rg);
        auto node = (rbs_types_function_param_t *)restPositionals;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }

    collectArgs(declaration, functionType->trailing_positionals, args);

    // Collect keywords

    collectKeywords(declaration, functionType->required_keywords, args);
    collectKeywords(declaration, functionType->optional_keywords, args);

    rbs_node_t *restKeywords = functionType->rest_keywords;
    if (restKeywords) {
        ENFORCE(restKeywords->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest keyword argument, expected `{}`", rbs_node_type_name(restKeywords),
                "FunctionParam");

        auto loc = declaration.typeLocFromRange(restKeywords->location->rg);
        auto node = (rbs_types_function_param_t *)restKeywords;
        auto arg = RBSArg{loc, node->name, node->type};
        args.emplace_back(arg);
    }

    // Collect block

    auto *block = node.block;
    if (block) {
        // TODO: RBS doesn't have location on blocks yet
        auto arg = RBSArg{fullTypeLoc, nullptr, (rbs_node_t *)block};
        args.emplace_back(arg);
    }

    auto sigParams = parser::NodeVec();
    sigParams.reserve(args.size());

    auto typeToParserNode = TypeToParserNode(ctx, typeParams, parser);

    auto methodArgs = getMethodArgs(methodDef);
    for (int i = 0; i < args.size(); i++) {
        auto &arg = args[i];
        auto type = typeToParserNode.toParserNode(arg.type, declaration);

        if (auto nameSymbol = arg.name) {
            // The RBS arg is named in the signature, so we use the explicit name used
            auto nameStr = parser.resolveConstant(nameSymbol);
            auto name = ctx.state.enterNameUTF8(nameStr);
            sigParams.emplace_back(make_unique<parser::Pair>(arg.loc, parser::MK::Symbol(arg.loc, name), move(type)));
        } else {
            if (!methodArgs || i >= methodArgs->args.size()) {
                if (auto e = ctx.beginError(fullTypeLoc, core::errors::Rewriter::RBSParameterMismatch)) {
                    e.setHeader("RBS signature has more parameters than in the method definition");
                }

                return nullptr;
            }

            // The RBS arg is not named in the signature, so we get it from the method definition
            auto methodArg = methodArgs->args[i].get();
            auto name = nodeName(methodArg);
            sigParams.emplace_back(
                make_unique<parser::Pair>(arg.loc, parser::MK::Symbol(methodArg->loc, name), move(type)));
        }
    }

    // Build the sig

    auto sigBuilder = parser::MK::Self(fullTypeLoc);

    if (abstract) {
        sigBuilder = parser::MK::Send0(fullTypeLoc, move(sigBuilder), core::Names::abstract(), fullTypeLoc);
    }

    sigBuilder = handleAnnotations(ctx, std::move(sigBuilder), annotations);

    if (typeParams.size() > 0) {
        auto typeParamsVector = parser::NodeVec();
        typeParamsVector.reserve(typeParams.size());

        for (auto &param : typeParams) {
            typeParamsVector.emplace_back(parser::MK::Symbol(param.first, param.second));
        }
        sigBuilder = parser::MK::Send(fullTypeLoc, move(sigBuilder), core::Names::typeParameters(), fullTypeLoc,
                                      move(typeParamsVector));
    }

    if (sigParams.size() > 0) {
        auto hash = parser::MK::Hash(fullTypeLoc, true, move(sigParams));
        auto args = parser::NodeVec();
        args.emplace_back(move(hash));
        sigBuilder = parser::MK::Send(fullTypeLoc, move(sigBuilder), core::Names::params(), fullTypeLoc, move(args));
    }

    rbs_node_t *returnValue = functionType->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        auto loc = declaration.typeLocFromRange(returnValue->location->rg);
        sigBuilder = parser::MK::Send0(fullTypeLoc, move(sigBuilder), core::Names::void_(), loc);
    } else {
        auto nameLoc = declaration.typeLocFromRange(returnValue->location->rg);
        auto returnType = typeToParserNode.toParserNode(returnValue, declaration);
        sigBuilder =
            parser::MK::Send1(fullTypeLoc, move(sigBuilder), core::Names::returns(), nameLoc, move(returnType));
    }

    auto sigArgs = parser::NodeVec();
    sigArgs.emplace_back(assembleTSigWithoutRuntime(firstLineTypeLoc));

    auto final = absl::c_find_if(annotations, [](const Comment &annotation) { return annotation.string == "final"; });
    if (final != annotations.end()) {
        sigArgs.emplace_back(parser::MK::Symbol(final->typeLoc, core::Names::final_()));
    }

    auto sig = parser::MK::Send(fullTypeLoc, parser::MK::SorbetPrivateStatic(fullTypeLoc), core::Names::sig(),
                                firstLineTypeLoc, move(sigArgs));

    return make_unique<parser::Block>(commentLoc, move(sig), nullptr, move(sigBuilder));
}

unique_ptr<parser::Node> MethodTypeToParserNode::attrSignature(const parser::Send *send, const rbs_node_t *type,
                                                               const RBSDeclaration &declaration,
                                                               const vector<Comment> &annotations) {
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>();

    auto fullTypeLoc = declaration.fullTypeLoc();
    auto firstLineTypeLoc = declaration.firstLineTypeLoc();
    auto commentLoc = declaration.commentLoc();

    auto sigBuilder = parser::MK::Self(fullTypeLoc.copyWithZeroLength());
    sigBuilder = handleAnnotations(ctx, std::move(sigBuilder), annotations);

    if (send->args.size() == 0) {
        if (auto e = ctx.beginError(send->loc, core::errors::Rewriter::RBSUnsupported)) {
            e.setHeader("RBS signatures do not support accessor without arguments");
        }

        return nullptr;
    }

    auto typeTranslator = TypeToParserNode(ctx, typeParams, parser);
    auto returnType = typeTranslator.toParserNode(type, declaration);

    if (send->method == core::Names::attrWriter()) {
        if (send->args.size() > 1) {
            if (auto e = ctx.beginError(send->loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for attr_writer do not support multiple arguments");
            }

            return nullptr;
        }

        // For attr writer, we need to add the param to the sig
        auto argName = nodeName(send->args[0].get());

        // The origin location points to the `:name` symbol, so we need to adjust it to point to the actual name
        auto argLoc = core::LocOffsets{
            send->args[0]->loc.beginPos() + 1,
            send->args[0]->loc.endPos(),
        };

        auto pairs = parser::NodeVec();
        pairs.emplace_back(make_unique<parser::Pair>(argLoc, parser::MK::Symbol(argLoc, argName),
                                                     typeTranslator.toParserNode(type, declaration)));
        auto hash = parser::MK::Hash(send->loc, true, move(pairs));
        auto sigArgs = parser::NodeVec();
        sigArgs.emplace_back(move(hash));
        sigBuilder = parser::MK::Send(send->loc, move(sigBuilder), core::Names::params(), send->loc, move(sigArgs));
    }

    sigBuilder =
        parser::MK::Send1(fullTypeLoc, move(sigBuilder), core::Names::returns(), returnType->loc, move(returnType));

    auto sigArgs = parser::NodeVec();
    sigArgs.emplace_back(assembleTSigWithoutRuntime(firstLineTypeLoc));

    auto final = absl::c_find_if(annotations, [](const Comment &annotation) { return annotation.string == "final"; });
    if (final != annotations.end()) {
        sigArgs.emplace_back(parser::MK::Symbol(final->typeLoc, core::Names::final_()));
    }

    auto sig = parser::MK::Send(fullTypeLoc, parser::MK::SorbetPrivateStatic(fullTypeLoc), core::Names::sig(),
                                firstLineTypeLoc, move(sigArgs));

    return make_unique<parser::Block>(commentLoc, move(sig), nullptr, move(sigBuilder));
}

pair<unique_ptr<parser::Node>, unique_ptr<parser::Node>>
MethodTypeToParserNode::methodDeclaration(const rbs_ast_members_method_definition_t *node,
                                          const RBSDeclaration &declaration) {
    auto declLoc = declaration.typeLocFromRange(node->base.location->rg);

    if (node->overloads->length > 1) {
        auto secondNode = node->overloads->head->next;
        auto secondLoc = declaration.typeLocFromRange(secondNode->node->location->rg);
        if (auto e = ctx.beginError(secondLoc, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("RBS signatures for abstract methods cannot have overloads");
        }

        return make_pair(nullptr, nullptr);
    }

    ENFORCE(node->overloads->head->node->type == RBS_AST_MEMBERS_METHOD_DEFINITION_OVERLOAD,
            "Unexpected node type `{}` in method definition, expected `{}`",
            rbs_node_type_name(node->overloads->head->node), "MethodDefinitionOverload");
    auto overload = (rbs_ast_members_method_definition_overload_t *)node->overloads->head->node;

    ENFORCE(overload->method_type->type == RBS_METHOD_TYPE,
            "Unexpected node type `{}` in method definition, expected `{}`", rbs_node_type_name(overload->method_type),
            "MethodType");
    auto methodType = (rbs_method_type_t *)overload->method_type;

    ENFORCE(methodType->type->type == RBS_TYPES_FUNCTION,
            "Unexpected node type `{}` in method definition, expected `{}`", rbs_node_type_name(methodType->type),
            "Function");
    auto functionType = (rbs_types_function_t *)methodType->type;

    auto defArgs = make_unique<parser::Args>(declLoc, parser::NodeVec());
    auto typeTranslator = TypeToParserNode(ctx, vector<pair<core::LocOffsets, core::NameRef>>(), parser);

    for (rbs_node_list_node_t *list_node = functionType->required_positionals->head; list_node != nullptr;
         list_node = list_node->next) {
        ENFORCE(list_node->node->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in required positional argument, expected `{}`",
                rbs_node_type_name(list_node->node), "FunctionParam");

        auto param = (rbs_types_function_param_t *)list_node->node;

        if (!param->name) {
            auto paramLoc = declaration.typeLocFromRange(param->type->location->rg);
            if (auto e = ctx.beginError(paramLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for abstract methods cannot have unnamed positional parameters");
            }

            return make_pair(nullptr, nullptr);
        }

        auto name = ctx.state.enterNameUTF8(parser.resolveConstant(param->name));
        auto nameLoc = declaration.typeLocFromRange(param->name->base.location->rg);
        defArgs->args.emplace_back(make_unique<parser::Arg>(nameLoc, name));
    }

    for (rbs_node_list_node_t *list_node = functionType->optional_positionals->head; list_node != nullptr;
         list_node = list_node->next) {
        ENFORCE(list_node->node->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in required positional argument, expected `{}`",
                rbs_node_type_name(list_node->node), "FunctionParam");

        auto param = (rbs_types_function_param_t *)list_node->node;

        if (!param->name) {
            auto paramLoc = declaration.typeLocFromRange(param->type->location->rg);
            if (auto e = ctx.beginError(paramLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for abstract methods cannot have unnamed positional parameters");
            }

            return make_pair(nullptr, nullptr);
        }

        auto name = ctx.state.enterNameUTF8(parser.resolveConstant(param->name));
        auto nameLoc = declaration.typeLocFromRange(param->name->base.location->rg);
        defArgs->args.emplace_back(make_unique<parser::Optarg>(nameLoc, name, nameLoc,
                                                               parser::MK::TUnsafe(nameLoc, parser::MK::Nil(nameLoc))));
    }

    if (functionType->rest_positionals) {
        ENFORCE(functionType->rest_positionals->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest positional argument, expected `{}`",
                rbs_node_type_name(functionType->rest_positionals), "FunctionParam");

        auto param = (rbs_types_function_param_t *)functionType->rest_positionals;

        if (param->name) {
            auto name = ctx.state.enterNameUTF8(parser.resolveConstant(param->name));
            auto nameLoc = declaration.typeLocFromRange(param->name->base.location->rg);
            defArgs->args.emplace_back(make_unique<parser::Restarg>(nameLoc, name, nameLoc));
        } else {
            auto paramLoc = declaration.typeLocFromRange(param->type->location->rg);
            if (auto e = ctx.beginError(paramLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for abstract methods cannot have unnamed positional parameters");
            }

            return make_pair(nullptr, nullptr);
        }
    }

    for (rbs_hash_node_t *hash_node = functionType->required_keywords->head; hash_node != nullptr;
         hash_node = hash_node->next) {
        ENFORCE(hash_node->key->type == RBS_AST_SYMBOL,
                "Unexpected node type `{}` in keyword argument name, expected `{}`", rbs_node_type_name(hash_node->key),
                "Symbol");

        auto name = ctx.state.enterNameUTF8(parser.resolveConstant((rbs_ast_symbol_t *)hash_node->key));
        auto nameLoc = declaration.typeLocFromRange(hash_node->key->location->rg);
        defArgs->args.emplace_back(make_unique<parser::Kwarg>(nameLoc, name));
    }

    for (rbs_hash_node_t *hash_node = functionType->optional_keywords->head; hash_node != nullptr;
         hash_node = hash_node->next) {
        ENFORCE(hash_node->key->type == RBS_AST_SYMBOL,
                "Unexpected node type `{}` in keyword argument name, expected `{}`", rbs_node_type_name(hash_node->key),
                "Symbol");

        auto name = ctx.state.enterNameUTF8(parser.resolveConstant((rbs_ast_symbol_t *)hash_node->key));
        auto nameLoc = declaration.typeLocFromRange(hash_node->key->location->rg);
        defArgs->args.emplace_back(make_unique<parser::Kwoptarg>(
            nameLoc, name, nameLoc, parser::MK::TUnsafe(nameLoc, parser::MK::Nil(nameLoc))));
    }

    if (functionType->rest_keywords) {
        ENFORCE(functionType->rest_keywords->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in rest keyword argument, expected `{}`",
                rbs_node_type_name(functionType->rest_keywords), "FunctionParam");

        auto param = (rbs_types_function_param_t *)functionType->rest_keywords;

        if (param->name) {
            auto name = ctx.state.enterNameUTF8(parser.resolveConstant(param->name));
            auto nameLoc = declaration.typeLocFromRange(param->name->base.location->rg);
            defArgs->args.emplace_back(make_unique<parser::Kwrestarg>(nameLoc, name));
        } else {
            auto paramLoc = declaration.typeLocFromRange(param->type->location->rg);
            if (auto e = ctx.beginError(paramLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for abstract methods cannot have unnamed keyword parameters");
            }

            return make_pair(nullptr, nullptr);
        }
    }

    if (methodType->block) {
        defArgs->args.emplace_back(make_unique<parser::Blockarg>(declLoc, core::Names::blkArg()));
    }

    // block

    unique_ptr<parser::Node> def = make_unique<parser::DefMethod>(
        declaration.fullTypeLoc(), declLoc, ctx.state.enterNameUTF8(parser.resolveConstant(node->name)), move(defArgs),
        nullptr);

    auto sig = methodSignature(def.get(), methodType, declaration, vector<Comment>(), true);

    if (node->visibility) {
        auto visibility = ctx.state.enterNameUTF8(parser.resolveGlobalConstant(node->visibility));
        if (visibility == core::Names::private_()) {
            auto args = parser::NodeVec();
            args.emplace_back(move(def));
            def = make_unique<parser::Send>(declLoc, parser::MK::Self(declLoc), core::Names::private_(), declLoc,
                                            move(args));
        }
    }

    return make_pair(move(sig), move(def));
}

} // namespace sorbet::rbs
