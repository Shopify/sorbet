#include "rbs/prism/MethodTypeToParserNodePrism.h"

#include "absl/algorithm/container.h"
#include "absl/strings/escaping.h"
#include "common/typecase.h"
#include "core/GlobalState.h"
#include "core/errors/internal.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "parser/prism/Helpers.h"
#include "rbs/TypeToParserNode.h"
#include "rbs/prism/TypeToParserNodePrism.h"
#include "rbs/rbs_method_common.h"
#include "rewriter/util/Util.h"
#include <cstring>

extern "C" {
#include "prism/util/pm_constant_pool.h"
}

using namespace std;
using namespace sorbet::parser::Prism;

namespace sorbet::rbs {

namespace {

// Forward declarations
// core::LocOffsets translateLocation(pm_location_t location);

core::LocOffsets adjustNameLoc(const RBSDeclaration &declaration, rbs_node_t *node) {
    auto range = node->location->rg;

    auto nameRange = node->location->children->entries[0].rg;
    if (!NULL_LOC_RANGE_P(nameRange)) {
        range.start.char_pos = nameRange.start;
        range.end.char_pos = nameRange.end;
    }

    return declaration.typeLocFromRange(range);
}

bool isSelfOrKernel(pm_node_t *node, const parser::Prism::Parser *prismParser) {
    if (PM_NODE_TYPE_P(node, PM_SELF_NODE)) {
        return true;
    }

    if (PM_NODE_TYPE_P(node, PM_CONSTANT_READ_NODE)) {
        auto *constant = down_cast<pm_constant_read_node_t>(node);
        auto name = prismParser->resolveConstant(constant->name);
        // Check if it's Kernel constant with no scope (::Kernel or bare Kernel)
        return name == "Kernel";
    }

    if (PM_NODE_TYPE_P(node, PM_CONSTANT_PATH_NODE)) {
        auto *constantPath = down_cast<pm_constant_path_node_t>(node);
        // Check if it's ::Kernel (parent is nullptr, representing root ::)
        // We reject Foo::Kernel or any other scoped constant
        if (constantPath->parent == nullptr) {
            auto name = prismParser->resolveConstant(constantPath->name);
            return name == "Kernel";
        }
    }

    return false;
}

bool isRaise(pm_node_t *node, const parser::Prism::Parser *prismParser) {
    if (!node) {
        return false;
    }

    // In Prism, method bodies are always wrapped in PM_STATEMENTS_NODE.
    // Unwrap if it contains exactly one statement (just the raise call).
    // Reject if multiple statements (e.g., puts + raise).
    if (PM_NODE_TYPE_P(node, PM_STATEMENTS_NODE)) {
        auto *stmts = down_cast<pm_statements_node_t>(node);
        if (stmts->body.size == 1) {
            node = stmts->body.nodes[0];
        } else {
            return false; // Multiple statements, not just a raise
        }
    }

    if (!PM_NODE_TYPE_P(node, PM_CALL_NODE)) {
        return false;
    }

    auto *call = down_cast<pm_call_node_t>(node);
    auto methodName = prismParser->resolveConstant(call->name);

    // TODO: Use nameref
    if (methodName != "raise") {
        return false;
    }

    // Check receiver is nil (implicit self) or self/Kernel
    return call->receiver == nullptr || isSelfOrKernel(call->receiver, prismParser);
}

core::AutocorrectSuggestion autocorrectAbstractBody(core::MutableContext ctx, const pm_node_t *method,
                                                    const parser::Prism::Parser *prismParser, pm_node_t *method_body) {
    core::LocOffsets editLoc;
    string corrected;

    auto *def = down_cast<pm_def_node_t>(const_cast<pm_node_t *>(method));
    auto methodLoc = prismParser->translateLocation(method->location);
    auto nameLoc = prismParser->translateLocation(def->name_loc);

    auto lineStart = core::Loc::pos2Detail(ctx.file.data(ctx), nameLoc.endPos()).line;
    auto lineEnd = core::Loc::pos2Detail(ctx.file.data(ctx), methodLoc.endPos()).line;

    if (method_body) {
        editLoc = prismParser->translateLocation(method_body->location);
        corrected = "raise \"Abstract method called\"";
    } else if (lineStart == lineEnd) {
        editLoc = nameLoc.copyEndWithZeroLength().join(methodLoc.copyEndWithZeroLength());
        corrected = " = raise(\"Abstract method called\")";
    } else {
        editLoc = nameLoc.copyEndWithZeroLength();
        auto [_endLoc, indentLength] = ctx.locAt(methodLoc).findStartOfIndentation(ctx);
        string indent(indentLength + 2, ' ');
        corrected = "\n" + indent + "raise \"Abstract method called\"";
    }

    return core::AutocorrectSuggestion{fmt::format("Add `raise` to the method body"),
                                       {core::AutocorrectSuggestion::Edit{ctx.locAt(editLoc), corrected}}};
}

void ensureAbstractMethodRaises(core::MutableContext ctx, const pm_node_t *node,
                                const parser::Prism::Parser *prismParser) {
    if (PM_NODE_TYPE_P(node, PM_DEF_NODE)) {
        auto *def = down_cast<pm_def_node_t>(const_cast<pm_node_t *>(node));
        if (def->body && isRaise(def->body, prismParser)) {
            def->body = nullptr;
            return;
        }

        auto nodeLoc = prismParser->translateLocation(node->location);

        if (auto e = ctx.beginIndexerError(nodeLoc, core::errors::Rewriter::RBSAbstractMethodNoRaises)) {
            e.setHeader("Methods declared @abstract with an RBS comment must always raise");
            auto autocorrect = autocorrectAbstractBody(ctx, node, prismParser, def->body);
            e.addAutocorrect(move(autocorrect));
        }
    }
}

pm_node_t *handleAnnotations(core::MutableContext ctx, const pm_node_t *node, pm_node_t *sigBuilder,
                             absl::Span<const Comment> annotations, const parser::Prism::Parser *prismParser,
                             const parser::Prism::Factory &prism) {
    for (auto &annotation : annotations) {
        if (annotation.string == "final") {
            // no-op, `final` is handled in the `sig()` call later
        } else if (annotation.string == "abstract") {
            sigBuilder = prism.Call0(annotation.typeLoc, sigBuilder, core::Names::abstract().show(ctx.state));
            ensureAbstractMethodRaises(ctx, node, prismParser);
        } else if (annotation.string == "overridable") {
            sigBuilder = prism.Call0(annotation.typeLoc, sigBuilder, core::Names::overridable().show(ctx.state));
        } else if (annotation.string == "override") {
            sigBuilder = prism.Call0(annotation.typeLoc, sigBuilder, core::Names::override_().show(ctx.state));
        } else if (annotation.string == "override(allow_incompatible: true)") {
            auto key = prism.Symbol(annotation.typeLoc, core::Names::allowIncompatible().show(ctx.state));
            auto value = prism.True(annotation.typeLoc);
            auto pair = prism.AssocNode(annotation.typeLoc, key, value);

            vector<pm_node_t *> pairs = {pair};
            auto hash = prism.KeywordHash(annotation.typeLoc, absl::MakeSpan(pairs));

            sigBuilder = prism.Call1(annotation.typeLoc, sigBuilder, core::Names::override_().show(ctx.state), hash);
        } else if (annotation.string == "override(allow_incompatible: :visibility)") {
            auto key = prism.Symbol(annotation.typeLoc, core::Names::allowIncompatible().show(ctx.state));
            auto value = prism.Symbol(annotation.typeLoc, core::Names::visibility().show(ctx.state));
            auto pair = prism.AssocNode(annotation.typeLoc, key, value);

            vector<pm_node_t *> pairs = {pair};
            auto hash = prism.KeywordHash(annotation.typeLoc, absl::MakeSpan(pairs));

            sigBuilder = prism.Call1(annotation.typeLoc, sigBuilder, core::Names::override_().show(ctx.state), hash);
        }
    }
    return sigBuilder;
}

/* TODO: Implement when needed
core::NameRef nodeName(const pm_node_t *node) {
    core::NameRef name;
    // TODO: Implement proper node name extraction for Prism nodes
    // This should handle PM_PARAMETER_NODE, PM_REQUIRED_PARAMETER_NODE, etc.
    (void)node; // Suppress unused warning for now
    return name;
}
*/

string nodeKindToString(const pm_node_t *node) {
    switch (PM_NODE_TYPE(node)) {
        case PM_REQUIRED_PARAMETER_NODE:
            return "positional";
        case PM_OPTIONAL_PARAMETER_NODE:
            return "optional positional";
        case PM_REST_PARAMETER_NODE:
            return "rest positional";
        case PM_REQUIRED_KEYWORD_PARAMETER_NODE:
            return "keyword";
        case PM_OPTIONAL_KEYWORD_PARAMETER_NODE:
            return "optional keyword";
        case PM_KEYWORD_REST_PARAMETER_NODE:
            return "rest keyword";
        case PM_BLOCK_PARAMETER_NODE:
            return "block";
        default:
            return "unknown";
    }
}

// core::LocOffsets translateLocation(pm_location_t location) {
//     // TODO: This should be shared with CommentsAssociatorPrism
//     // Use proper pointer arithmetic for location translation
//     const uint8_t *sourceStart = location.start;
//     const uint8_t *sourceEnd = location.end;
//     uint32_t start = static_cast<uint32_t>(sourceStart - sourceStart); // This will be 0 for now
//     uint32_t end = static_cast<uint32_t>(sourceEnd - sourceStart);
//     return core::LocOffsets{start, end};
// }

optional<core::AutocorrectSuggestion> autocorrectArg(core::MutableContext ctx, pm_node_t *methodArg, RBSArg arg,
                                                     const parser::Prism::Parser &prismParser,
                                                     const RBSDeclaration &declaration) {
    if (arg.kind == RBSArg::Kind::Block || PM_NODE_TYPE_P(methodArg, PM_BLOCK_PARAMETER_NODE)) {
        // Block arguments are not autocorrected
        return nullopt;
    }

    string corrected;
    auto source = ctx.file.data(ctx.state).source();

    // Note: Whitequark's autocorrectArg takes unique_ptr<parser::Node> type and extracts from type->loc.
    // In Prism, we cannot pass the converted typeNode because toPrismNode() creates synthesized nodes
    // with new locations for the signature, not nodes that preserve the original RBS source locations.
    // Instead, we extract directly from the RBS source using declaration.typeLocFromRange(arg.type->location->rg).
    auto typeLoc = declaration.typeLocFromRange(arg.type->location->rg);
    string typeString = string(source.substr(typeLoc.beginPos(), typeLoc.endPos() - typeLoc.beginPos()));

    switch (PM_NODE_TYPE(methodArg)) {
        // Should be: `Type name`
        case PM_REQUIRED_PARAMETER_NODE: {
            auto *param = down_cast<pm_required_parameter_node_t>(methodArg);
            if (arg.name) {
                auto nameStr = prismParser.resolveConstant(param->name);
                corrected = fmt::format("{} {}", typeString, nameStr);
            } else {
                corrected = fmt::format("{}", typeString);
            }
            break;
        }
        // Should be: `?Type name`
        case PM_OPTIONAL_PARAMETER_NODE: {
            auto *param = down_cast<pm_optional_parameter_node_t>(methodArg);
            if (arg.name) {
                auto nameStr = prismParser.resolveConstant(param->name);
                corrected = fmt::format("?{} {}", typeString, nameStr);
            } else {
                corrected = fmt::format("?{}", typeString);
            }
            break;
        }
        // Should be: `*Type name`
        case PM_REST_PARAMETER_NODE: {
            auto *param = down_cast<pm_rest_parameter_node_t>(methodArg);
            if (arg.name) {
                auto nameStr = prismParser.resolveConstant(param->name);
                corrected = fmt::format("*{} {}", typeString, nameStr);
            } else {
                corrected = fmt::format("*{}", typeString);
            }
            break;
        }
        // Should be: `name: Type`
        case PM_REQUIRED_KEYWORD_PARAMETER_NODE: {
            auto *param = down_cast<pm_required_keyword_parameter_node_t>(methodArg);
            auto nameStr = prismParser.resolveConstant(param->name);
            corrected = fmt::format("{}: {}", nameStr, typeString);
            break;
        }
        // Should be: `?name: Type`
        case PM_OPTIONAL_KEYWORD_PARAMETER_NODE: {
            auto *param = down_cast<pm_optional_keyword_parameter_node_t>(methodArg);
            auto nameStr = prismParser.resolveConstant(param->name);
            corrected = fmt::format("?{}: {}", nameStr, typeString);
            break;
        }
        // Should be: `**Type name`
        case PM_KEYWORD_REST_PARAMETER_NODE: {
            auto *param = down_cast<pm_keyword_rest_parameter_node_t>(methodArg);
            if (arg.name) {
                auto nameStr = prismParser.resolveConstant(param->name);
                corrected = fmt::format("**{} {}", typeString, nameStr);
            } else {
                corrected = fmt::format("**{}", typeString);
            }
            break;
        }
        default:
            return nullopt;
    }

    core::LocOffsets loc = arg.loc;

    // Adjust the location to account for the autocorrect
    if (arg.kind == RBSArg::Kind::OptionalPositional || arg.kind == RBSArg::Kind::RestPositional ||
        arg.kind == RBSArg::Kind::OptionalKeyword) {
        loc.beginLoc -= 1;
    } else if (arg.kind == RBSArg::Kind::RestKeyword) {
        loc.beginLoc -= 2;
    }

    return core::AutocorrectSuggestion{fmt::format("Replace with `{}`", argKindToString(arg.kind)),
                                       {core::AutocorrectSuggestion::Edit{ctx.locAt(loc), corrected}}};
}

bool checkParameterKindMatch(const RBSArg &arg, const pm_node_t *methodArg) {
    switch (PM_NODE_TYPE(methodArg)) {
        case PM_REQUIRED_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::Positional;
        case PM_OPTIONAL_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::OptionalPositional;
        case PM_REST_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::RestPositional;
        case PM_REQUIRED_KEYWORD_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::Keyword;
        case PM_OPTIONAL_KEYWORD_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::OptionalKeyword;
        case PM_KEYWORD_REST_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::RestKeyword;
        case PM_BLOCK_PARAMETER_NODE:
            return arg.kind == RBSArg::Kind::Block;
        default:
            return false;
    }
}

/* TODO: Implement when needed
parser::Args *getMethodArgs(const pm_node_t *node) {
    if (PM_NODE_TYPE_P(node, PM_DEF_NODE)) {
        auto *def = down_cast<pm_def_node_t>(const_cast<pm_node_t *>(node));
        // TODO: Convert Prism parameters to parser::Args
        // For now, return nullptr to indicate no args
        (void)def; // Suppress unused warning
        return nullptr;
    }
    return nullptr;
}
*/

void collectArgs(const RBSDeclaration &declaration, rbs_node_list_t *field, vector<RBSArg> &args, RBSArg::Kind kind) {
    if (field == nullptr || field->length == 0) {
        return;
    }

    for (rbs_node_list_node_t *list_node = field->head; list_node != nullptr; list_node = list_node->next) {
        auto loc = declaration.typeLocFromRange(list_node->node->location->rg);
        auto nameLoc = adjustNameLoc(declaration, list_node->node);

        ENFORCE(list_node->node->type == RBS_TYPES_FUNCTION_PARAM,
                "Unexpected node type `{}` in function parameter, expected `{}`", rbs_node_type_name(list_node->node),
                "FunctionParam");

        auto *param = rbs_down_cast<rbs_types_function_param_t>(list_node->node);
        auto arg = RBSArg{loc, nameLoc, param->name, param->type, kind};
        args.emplace_back(arg);
    }
}

void collectKeywords(const RBSDeclaration &declaration, rbs_hash_t *field, vector<RBSArg> &args, RBSArg::Kind kind) {
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

        auto nameLoc = declaration.typeLocFromRange(hash_node->key->location->rg);
        auto loc = nameLoc.join(declaration.typeLocFromRange(hash_node->value->location->rg));
        rbs_ast_symbol_t *keyNode = rbs_down_cast<rbs_ast_symbol_t>(hash_node->key);
        rbs_types_function_param_t *valueNode = rbs_down_cast<rbs_types_function_param_t>(hash_node->value);
        auto arg = RBSArg{loc, nameLoc, keyNode, valueNode->type, kind};
        args.emplace_back(arg);
    }
}

} // namespace

// Collects method parameter names (in positional/keyword/block order) from a Prism def node
namespace {
struct MethodParamInfo {
    pm_constant_id_t nameId;
    pm_node_t *node;
};

void appendParamName(vector<MethodParamInfo> &out, pm_node_t *paramNode) {
    if (paramNode == nullptr) {
        return;
    }

    switch (PM_NODE_TYPE(paramNode)) {
        case PM_REQUIRED_PARAMETER_NODE: {
            auto *n = down_cast<pm_required_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_OPTIONAL_PARAMETER_NODE: {
            auto *n = down_cast<pm_optional_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_REST_PARAMETER_NODE: {
            auto *n = down_cast<pm_rest_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_REQUIRED_KEYWORD_PARAMETER_NODE: {
            auto *n = down_cast<pm_required_keyword_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_OPTIONAL_KEYWORD_PARAMETER_NODE: {
            auto *n = down_cast<pm_optional_keyword_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_KEYWORD_REST_PARAMETER_NODE: {
            auto *n = down_cast<pm_keyword_rest_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        case PM_BLOCK_PARAMETER_NODE: {
            auto *n = down_cast<pm_block_parameter_node_t>(paramNode);
            out.push_back(MethodParamInfo{n->name, paramNode});
            break;
        }
        default:
            break;
    }
}

vector<MethodParamInfo> getMethodArgs(pm_def_node_t *def) {
    vector<MethodParamInfo> result;
    if (!def || def->parameters == nullptr) {
        return result;
    }

    auto *params = def->parameters;

    // requireds
    if (params->requireds.size > 0) {
        for (size_t i = 0; i < params->requireds.size; i++) {
            appendParamName(result, params->requireds.nodes[i]);
        }
    }
    // optionals
    if (params->optionals.size > 0) {
        for (size_t i = 0; i < params->optionals.size; i++) {
            appendParamName(result, params->optionals.nodes[i]);
        }
    }
    // rest
    appendParamName(result, params->rest);
    // posts (trailing positionals)
    if (params->posts.size > 0) {
        for (size_t i = 0; i < params->posts.size; i++) {
            appendParamName(result, params->posts.nodes[i]);
        }
    }
    // keywords
    if (params->keywords.size > 0) {
        for (size_t i = 0; i < params->keywords.size; i++) {
            appendParamName(result, params->keywords.nodes[i]);
        }
    }
    // keyword rest
    appendParamName(result, params->keyword_rest);
    // block
    appendParamName(result, up_cast(params->block));

    return result;
}
} // namespace

// (Removed minimal type builders that accessed private members)

pm_node_t *MethodTypeToParserNodePrism::attrSignature(const pm_call_node_t *call, const rbs_node_t *type,
                                                      const RBSDeclaration &declaration,
                                                      absl::Span<const Comment> annotations) {
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>{};
    auto fullTypeLoc = declaration.fullTypeLoc();
    auto firstLineTypeLoc = declaration.firstLineTypeLoc();
    auto commentLoc = declaration.commentLoc();

    pm_node_t *sigBuilder = prism.Self(fullTypeLoc.copyWithZeroLength());
    sigBuilder =
        handleAnnotations(ctx, const_cast<pm_node_t *>(&call->base), sigBuilder, annotations, &prismParser, prism);

    if (call->arguments == nullptr || call->arguments->arguments.size == 0) {
        if (auto e = ctx.beginIndexerError(prismParser.translateLocation(call->base.location),
                                           core::errors::Rewriter::RBSUnsupported)) {
            e.setHeader("RBS signatures do not support accessor without arguments");
        }
        return nullptr;
    }

    auto typeTranslator = TypeToParserNodePrism(ctx, typeParams, parser, prismParser);

    auto methodName = prismParser.resolveConstant(call->name);

    if (methodName == "attr_writer") {
        // Create "params" call

        if (call->arguments->arguments.size > 1) {
            if (auto e = ctx.beginIndexerError(prismParser.translateLocation(call->base.location),
                                               core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS signatures for attr_writer do not support multiple arguments");
            }
            return nullptr;
        }

        // For attr_writer, we need to add the param to the sig
        pm_node_t *arg = call->arguments->arguments.nodes[0];
        if (!PM_NODE_TYPE_P(arg, PM_SYMBOL_NODE)) {
            return nullptr;
        }
        auto *symbolNode = down_cast<pm_symbol_node_t>(arg);
        auto argName = string{(const char *)symbolNode->unescaped.source, symbolNode->unescaped.length};

        // The location points to the `:name` symbol, adjust to point to actual name
        auto argLoc = prismParser.translateLocation(arg->location);
        argLoc = core::LocOffsets{argLoc.beginPos() + 1, argLoc.endPos()};

        pm_node_t *keyNode = prism.Symbol(argLoc, argName);
        pm_node_t *paramType = typeTranslator.toPrismNode(type, declaration);
        pm_node_t *assoc = prism.AssocNode(prismParser.translateLocation(call->base.location), keyNode, paramType);

        auto hashElements = std::array{assoc};
        pm_node_t *hash =
            prism.KeywordHash(prismParser.translateLocation(call->base.location), absl::MakeSpan(hashElements));
        sigBuilder = prism.Call1(prismParser.translateLocation(call->base.location), sigBuilder, "params"sv, hash);
    }

    // Add returns(Type) call
    pm_node_t *returnType = typeTranslator.toPrismNode(type, declaration);
    sigBuilder = prism.Call1(fullTypeLoc, sigBuilder, "returns"sv, returnType);

    // Create sig call arguments
    vector<pm_node_t *> sigArgs;
    auto finalAnnotation =
        absl::c_find_if(annotations, [](const Comment &annotation) { return annotation.string == "final"; });
    if (finalAnnotation != annotations.end()) {
        sigArgs.push_back(prism.Symbol(finalAnnotation->typeLoc, "final"sv));
    }

    pm_node_t *sigReceiver = prism.TSigWithoutRuntime(firstLineTypeLoc);

    // Create block node with sigBuilder as body
    pm_node_t *block = prism.Block(commentLoc, sigBuilder);

    // Create sig call with block
    pm_node_t *sigCall = prism.Call(fullTypeLoc, sigReceiver, "sig"sv, absl::MakeSpan(sigArgs), block);

    return sigCall;
}

pm_node_t *MethodTypeToParserNodePrism::methodSignature(const pm_node_t *methodDef, const rbs_method_type_t *methodType,
                                                        const RBSDeclaration &declaration,
                                                        absl::Span<const Comment> annotations) {
    // fmt::print("DEBUG: MethodTypeToParserNodePrism::methodSignature called\n");

    // Parse the RBS method type and create appropriate signature nodes
    const auto &node = *methodType;

    // Collect type parameters
    vector<pair<core::LocOffsets, core::NameRef>> typeParams;
    for (rbs_node_list_node_t *list_node = node.type_params->head; list_node != nullptr; list_node = list_node->next) {
        auto loc = declaration.typeLocFromRange(list_node->node->location->rg);

        ENFORCE(list_node->node->type == RBS_AST_TYPE_PARAM,
                "Unexpected node type `{}` in type parameter list, expected `{}`", rbs_node_type_name(list_node->node),
                "TypeParam");

        auto node = rbs_down_cast<rbs_ast_type_param_t>(list_node->node);
        auto str = parser.resolveConstant(node->name);
        typeParams.emplace_back(loc, ctx.state.enterNameUTF8(str));
    }

    // Validate that we have a function type
    if (node.type->type != RBS_TYPES_FUNCTION) {
        auto errLoc = declaration.typeLocFromRange(node.type->location->rg);
        if (auto e = ctx.beginIndexerError(errLoc, core::errors::Rewriter::RBSUnsupported)) {
            e.setHeader("Unexpected node type `{}` in method signature, expected `{}`", rbs_node_type_name(node.type),
                        "Function");
        }
        return nullptr;
    }

    auto *functionType = rbs_down_cast<rbs_types_function_t>(node.type);

    (void)methodDef; // Suppress unused warning for now

    // Collect RBS parameters for sig params
    vector<RBSArg> args;
    collectArgs(declaration, functionType->required_positionals, args, RBSArg::Kind::Positional);

    collectArgs(declaration, functionType->optional_positionals, args, RBSArg::Kind::OptionalPositional);
    if (functionType->rest_positionals) {
        auto loc = declaration.typeLocFromRange(functionType->rest_positionals->location->rg);
        auto nameLoc = adjustNameLoc(declaration, functionType->rest_positionals);
        auto *param = rbs_down_cast<rbs_types_function_param_t>(functionType->rest_positionals);
        auto arg = RBSArg{loc, nameLoc, param->name, param->type, RBSArg::Kind::RestPositional};
        args.emplace_back(arg);
    }
    // Include trailing positionals to match non-Prism behavior
    collectArgs(declaration, functionType->trailing_positionals, args, RBSArg::Kind::Positional);
    collectKeywords(declaration, functionType->required_keywords, args, RBSArg::Kind::Keyword);
    collectKeywords(declaration, functionType->optional_keywords, args, RBSArg::Kind::OptionalKeyword);
    if (functionType->rest_keywords) {
        auto loc = declaration.typeLocFromRange(functionType->rest_keywords->location->rg);
        auto nameLoc = adjustNameLoc(declaration, functionType->rest_keywords);
        auto *param = rbs_down_cast<rbs_types_function_param_t>(functionType->rest_keywords);
        auto arg = RBSArg{loc, nameLoc, param->name, param->type, RBSArg::Kind::RestKeyword};
        args.emplace_back(arg);
    }
    auto *rbsBlock = node.block;
    if (rbsBlock) {
        auto loc = declaration.typeLocFromRange(rbsBlock->base.location->rg);
        auto arg = RBSArg{loc, loc, nullptr, (rbs_node_t *)rbsBlock, RBSArg::Kind::Block};
        args.emplace_back(arg);
    }

    auto fullTypeLoc = declaration.fullTypeLoc();
    auto firstLineTypeLoc = declaration.firstLineTypeLoc();
    auto commentLoc = declaration.commentLoc();

    // Create receiver: T::Sig::WithoutRuntime
    pm_node_t *receiver = prism.TSigWithoutRuntime(firstLineTypeLoc);

    // Create sig parameter pairs for .params() call (keyword arguments)
    vector<pm_node_t *> sigParams;
    sigParams.reserve(args.size());

    // Collect Ruby method parameter names once (mirror WQ)
    vector<MethodParamInfo> methodArgs;
    if (PM_NODE_TYPE_P((pm_node_t *)methodDef, PM_DEF_NODE)) {
        auto def = down_cast<pm_def_node_t>((pm_node_t *)methodDef);
        methodArgs = getMethodArgs(def);
    }

    // Create type converter for RBS types to Prism nodes
    auto typeToPrismNode = TypeToParserNodePrism(ctx, typeParams, parser, prismParser);

    size_t paramIndex = 0;
    for (const auto &arg : args) {
        // fmt::print("DEBUG: Processing arg, hasName={}, kind={}\n", (arg.name != nullptr),
        // static_cast<int>(arg.kind));

        // Create type node from RBS type
        pm_node_t *typeNode = typeToPrismNode.toPrismNode(arg.type, declaration);
        if (!typeNode) {
            continue;
        }

        if (methodArgs.empty() || paramIndex >= methodArgs.size()) {
            if (auto e = ctx.beginIndexerError(fullTypeLoc, core::errors::Rewriter::RBSParameterMismatch)) {
                e.setHeader("RBS signature has more parameters than in the method definition");
            }

            return nullptr;
        }

        // Check parameter kind match and generate autocorrect if needed
        pm_node_t *methodArg = methodArgs[paramIndex].node;
        if (!checkParameterKindMatch(arg, methodArg)) {
            if (auto e = ctx.beginIndexerError(arg.loc, core::errors::Rewriter::RBSIncorrectParameterKind)) {
                auto methodArgNameStr = prismParser.resolveConstant(methodArgs[paramIndex].nameId);
                e.setHeader("Argument kind mismatch for `{}`, method declares `{}`, but RBS signature declares `{}`",
                            methodArgNameStr, nodeKindToString(methodArg), argKindToString(arg.kind));

                e.maybeAddAutocorrect(autocorrectArg(ctx, methodArg, arg, prismParser, declaration));
            }
        }

        // Create symbol node for parameter name
        pm_node_t *symbolNode = nullptr;
        if (arg.name) {
            symbolNode = createSymbolNode(arg.name, arg.nameLoc);
        } else {
            // Fallback to method parameter name when RBS omitted it
            core::LocOffsets tinyLocOffsets = firstLineTypeLoc.copyWithZeroLength();
            if (!methodArgs.empty() && paramIndex < methodArgs.size()) {
                auto methodArg = methodArgs[paramIndex];

                if (methodArg.nameId == PM_CONSTANT_ID_UNSET) {
                    switch (arg.kind) {
                        case RBSArg::Kind::RestKeyword:
                            symbolNode = prism.Symbol(tinyLocOffsets, "**"sv);
                            break;
                        case RBSArg::Kind::RestPositional:
                            symbolNode = prism.Symbol(tinyLocOffsets, "*"sv);
                            break;
                        case RBSArg::Kind::Block:
                            symbolNode = prism.Symbol(tinyLocOffsets, "&"sv);
                            break;
                        default:
                            break;
                    }
                } else {
                    symbolNode = prism.SymbolFromConstant(tinyLocOffsets, methodArg.nameId);
                }
            }
            if (!symbolNode) {
                // As a last resort, synthesize a symbol ':arg'
                symbolNode = prism.Symbol(tinyLocOffsets, "arg"sv);
            }
        }
        if (!symbolNode) {
            continue;
        }

        // Create association node (key-value pair) for keyword argument
        core::LocOffsets tinyLocOffsets = firstLineTypeLoc.copyWithZeroLength();
        pm_node_t *pairNode = prism.AssocNode(tinyLocOffsets, symbolNode, typeNode);
        sigParams.push_back(pairNode);
        paramIndex++;
    }

    // Build sig chain
    pm_node_t *sigReceiver = prism.Self(fullTypeLoc);

    // Add annotations (abstract, override, etc.)
    sigReceiver = handleAnnotations(ctx, methodDef, sigReceiver, annotations, &prismParser, prism);

    // Add .type_parameters() call if we have type parameters
    if (typeParams.size() > 0) {
        vector<pm_node_t *> typeParamSymbols;
        typeParamSymbols.reserve(typeParams.size());

        for (auto &param : typeParams) {
            string nameStr = param.second.show(ctx.state);
            pm_node_t *symbolNode = prism.Symbol(param.first, nameStr);
            typeParamSymbols.push_back(symbolNode);
        }

        pm_node_t *typeParamsCall =
            prism.Call(fullTypeLoc, sigReceiver, "type_parameters"sv, absl::MakeSpan(typeParamSymbols));
        ENFORCE(typeParamsCall, "Failed to create type parameters call");

        sigReceiver = typeParamsCall;
    }

    // Add .params() call if we have parameters
    if (sigParams.size() > 0) {
        // Wrap pairs in KeywordHashNode for keyword arguments
        core::LocOffsets hashLoc = fullTypeLoc;
        pm_node_t *paramsHash = prism.KeywordHash(hashLoc, absl::MakeSpan(sigParams));

        // Create .params() method call with keyword hash
        pm_node_t *paramsCall = prism.Call1(fullTypeLoc, sigReceiver, "params"sv, paramsHash);

        sigReceiver = paramsCall;
    }

    // Add return type call (.void() or .returns(Type))
    pm_node_t *blockBody = nullptr;

    // Pre-calculate return type location for setting on the return type node
    pm_location_t return_type_full_loc =
        prismParser.convertLocOffsets(declaration.typeLocFromRange(functionType->return_type->location->rg));

    if (functionType->return_type->type == RBS_TYPES_BASES_VOID) {
        // Create: sigReceiver.void()
        blockBody =
            prism.Call0(declaration.typeLocFromRange(functionType->return_type->location->rg), sigReceiver, "void"sv);
        // debugPrintLocation("void.call.base", voidCall->base.location);
        // debugPrintLocation("void.call.msg", voidCall->message_loc);
    } else {
        // Create: sigReceiver.returns(Type)
        // Convert actual return type from RBS AST
        pm_node_t *returnTypeNode = typeToPrismNode.toPrismNode(functionType->return_type, declaration);
        ENFORCE(returnTypeNode, "Failed to create return type node");

        // Set return type node base.location to actual return type span
        returnTypeNode->location = return_type_full_loc;

        blockBody = prism.Call1(declaration.typeLocFromRange(functionType->return_type->location->rg), sigReceiver,
                                "returns"sv, returnTypeNode);
        // debugPrintLocation("returns.call.base", returnsCall->base.location);
        // debugPrintLocation("returns.call.msg", returnsCall->message_loc);
    }

    pm_node_t *block = prism.Block(fullTypeLoc, blockBody);

    vector<pm_node_t *> sig_args;

    // Check for @final annotation and add :final as argument if present
    auto final = absl::c_find_if(annotations, [](const Comment &annotation) { return annotation.string == "final"; });
    if (final != annotations.end()) {
        pm_node_t *finalSymbol = prism.Symbol(final->typeLoc, core::Names::final_().show(ctx.state));
        if (finalSymbol) {
            sig_args.push_back(finalSymbol);
        }
    }

    pm_node_t *call = prism.Call(fullTypeLoc, receiver, "sig"sv, absl::MakeSpan(sig_args), block);

    // Debug print important locations to diagnose substr crashes
    // debugPrintLocation("sig.call.base", call->base.location);
    // debugPrintLocation("sig.call.msg", call->message_loc);
    // debugPrintLocation("block.open", block->opening_loc);
    // debugPrintLocation("block.close", block->closing_loc);

    (void)commentLoc; // Suppress unused warning
    return call;
}

pm_node_t *MethodTypeToParserNodePrism::createSymbolNode(rbs_ast_symbol_t *name, core::LocOffsets nameLoc) {
    if (!name) {
        return nullptr;
    }

    // Convert RBS symbol to string and use shared helper
    auto nameStr = parser.resolveConstant(name);
    string nameString(nameStr);

    return prism.Symbol(nameLoc, nameString);
}

} // namespace sorbet::rbs
