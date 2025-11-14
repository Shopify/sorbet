#include "rbs/prism/TypeToParserNodePrism.h"

#include "common/typecase.h"
#include "core/GlobalState.h"
#include "core/errors/internal.h"
#include "core/errors/rewriter.h"
#include "parser/prism/Helpers.h"

using namespace std;
using namespace sorbet::parser::Prism;

namespace sorbet::rbs {

namespace {

bool hasTypeParam(absl::Span<const pair<core::LocOffsets, core::NameRef>> typeParams, core::NameRef name) {
    return absl::c_any_of(typeParams, [&](const auto &param) { return param.second == name; });
}

} // namespace

pm_node_t *TypeToParserNodePrism::namespaceConst(const rbs_namespace_t *rbsNamespace, const RBSDeclaration &declaration,
                                                 core::LocOffsets loc) {
    rbs_node_list *typePath = rbsNamespace->path;

    pm_node_t *parent = nullptr;

    if (rbsNamespace->absolute) {
        // Create root constant access (::)
        parent = prism.ConstantReadNode(""sv, loc);
    }

    if (typePath != nullptr) {
        for (rbs_node_list_node *list_node = typePath->head; list_node != nullptr; list_node = list_node->next) {
            rbs_node_t *node = list_node->node;

            ENFORCE(node->type == RBS_AST_SYMBOL, "Unexpected node type `{}` in type name, expected `{}`",
                    rbs_node_type_name(node), "Symbol");

            rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node;
            auto nameStr = parser.resolveConstant(symbol);
            string nameString(nameStr);

            pm_node_t *constNode = prism.ConstantReadNode(nameString, loc);
            if (parent) {
                // Create constant path node for scoped access
                // This is simplified - full implementation would use pm_constant_path_node_create
                parent = constNode;
            } else {
                parent = constNode;
            }
        }
    }

    return parent;
}

pm_node_t *TypeToParserNodePrism::typeNameType(const rbs_type_name_t *typeName, bool isGeneric,
                                               const RBSDeclaration &declaration) {
    auto loc = declaration.typeLocFromRange(((rbs_node_t *)typeName)->location->rg);

    pm_node_t *parent = namespaceConst(typeName->rbs_namespace, declaration, loc);

    auto nameStr = parser.resolveConstant(typeName->name);
    auto nameUTF8 = ctx.state.enterNameUTF8(nameStr);
    auto nameConstant = ctx.state.enterNameConstant(nameUTF8);

    if (isGeneric) {
        if (!parent) { // Root level constants
            if (nameConstant == core::Names::Constants::Array()) {
                return prism.T_Array(loc);
            } else if (nameConstant == core::Names::Constants::Class()) {
                return prism.T_Class(loc);
            } else if (nameConstant == core::Names::Constants::Enumerable()) {
                return prism.T_Enumerable(loc);
            } else if (nameConstant == core::Names::Constants::Enumerator()) {
                return prism.T_Enumerator(loc);
            } else if (nameConstant == core::Names::Constants::Hash()) {
                return prism.T_Hash(loc);
            } else if (nameConstant == core::Names::Constants::Set()) {
                return prism.T_Set(loc);
            } else if (nameConstant == core::Names::Constants::Range()) {
                return prism.T_Range(loc);
            }
        }
    } else {
        // The type may refer to a type parameter, so we need to check if it exists as a NameKind::UTF8
        if (hasTypeParam(typeParams, nameUTF8)) {
            // Ensure proper string conversion from string_view
            string nameString{nameStr.data(), nameStr.size()};
            pm_node_t *symbolNode = prism.Symbol(loc, nameString);
            return prism.TTypeParameter(loc, symbolNode);
        }
    }

    // Create a proper constant path node (parent::name or just name if no parent)
    string nameString{nameStr.data(), nameStr.size()};
    if (parent) {
        return prism.ConstantPathNode(loc, parent, nameString);
    } else {
        return prism.ConstantReadNode(nameString, loc);
    }
}

pm_node_t *TypeToParserNodePrism::aliasType(const rbs_types_alias_t *node, core::LocOffsets loc,
                                            const RBSDeclaration &declaration) {
    auto nameView = parser.resolveConstant(node->name->name);
    auto nameStr = "type " + string(nameView);

    // addConstantToPool copies the string, so it's safe to pass a temporary
    return prism.ConstantReadNode(nameStr, loc);
}

pm_node_t *TypeToParserNodePrism::classInstanceType(const rbs_types_class_instance_t *node, core::LocOffsets loc,
                                                    const RBSDeclaration &declaration) {
    // fmt::print("TypeToParserNodePrism::classInstanceType\n");
    auto argsValue = node->args;
    auto isGeneric = argsValue != nullptr && argsValue->length > 0;
    auto typeConstant = typeNameType(node->name, isGeneric, declaration);

    if (isGeneric) {
        vector<pm_node_t *> args;
        args.reserve(argsValue->length);
        for (rbs_node_list_node *list_node = argsValue->head; list_node != nullptr; list_node = list_node->next) {
            auto argType = toPrismNode(list_node->node, declaration);
            args.push_back(argType);
        }

        string methodName = core::Names::syntheticSquareBrackets().show(ctx.state);
        return prism.Send(loc, typeConstant, methodName.c_str(), args);
    }

    return typeConstant;
}

pm_node_t *TypeToParserNodePrism::classSingletonType(const rbs_types_class_singleton_t *node, core::LocOffsets loc,
                                                     const RBSDeclaration &declaration) {
    // Simplified - should create T.class_of call
    return prism.ConstantReadNode("T.class_of"sv, loc);
}

pm_node_t *TypeToParserNodePrism::unionType(const rbs_types_union_t *node, core::LocOffsets loc,
                                            const RBSDeclaration &declaration) {
    vector<pm_node_t *> args;
    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = toPrismNode(list_node->node, declaration);
        args.push_back(innerType);
    }
    return prism.TAny(loc, args);
}

pm_node_t *TypeToParserNodePrism::intersectionType(const rbs_types_intersection_t *node, core::LocOffsets loc,
                                                   const RBSDeclaration &declaration) {
    vector<pm_node_t *> args;
    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = toPrismNode(list_node->node, declaration);
        args.push_back(innerType);
    }
    return prism.TAll(loc, args);
}

pm_node_t *TypeToParserNodePrism::optionalType(const rbs_types_optional_t *node, core::LocOffsets loc,
                                               const RBSDeclaration &declaration) {
    auto innerType = toPrismNode(node->type, declaration);
    if (prismParser.isTUntyped(innerType)) {
        return innerType;
    }
    return prism.TNilable(loc, innerType);
}

pm_node_t *TypeToParserNodePrism::voidType(const rbs_types_bases_void_t *node, core::LocOffsets loc) {
    return prism.ConstantReadNode("T.void"sv, loc);
}

pm_node_t *TypeToParserNodePrism::functionType(const rbs_types_function_t *node, core::LocOffsets loc,
                                               const RBSDeclaration &declaration) {
    vector<pm_node_t *> pairs;
    int i = 0;
    for (rbs_node_list_node *list_node = node->required_positionals->head; list_node != nullptr;
         list_node = list_node->next) {
        string argNameStr = "arg" + to_string(i);
        auto key = prism.Symbol(loc, argNameStr.c_str());

        rbs_node_t *paramNode = list_node->node;
        pm_node_t *innerType;

        if (paramNode->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginIndexerError(loc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in function parameter type, expected `{}`",
                            rbs_node_type_name(paramNode), "FunctionParam");
            }
            innerType = prism.TUntyped(loc);
        } else {
            innerType = toPrismNode(((rbs_types_function_param_t *)paramNode)->type, declaration);
        }

        pm_node_t *pairNode = prism.AssocNode(loc, key, innerType);
        pairs.push_back(pairNode);

        i++;
    }

    rbs_node_t *returnValue = node->return_type;
    pm_node_t *argsHash = pairs.empty() ? nullptr : prism.KeywordHash(loc, pairs);

    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        return prism.TProcVoid(loc, argsHash);
    }

    auto returnType = toPrismNode(returnValue, declaration);
    return prism.TProc(loc, argsHash, returnType);
}

pm_node_t *TypeToParserNodePrism::procType(const rbs_types_proc_t *node, core::LocOffsets loc,
                                           const RBSDeclaration &declaration) {
    return functionType((rbs_types_function_t *)node->type, loc, declaration);
}

pm_node_t *TypeToParserNodePrism::blockType(const rbs_types_block_t *node, core::LocOffsets loc,
                                            const RBSDeclaration &declaration) {
    pm_node_t *function = prism.TUntyped(loc);

    rbs_node_t *functionTypeNode = node->type;
    switch (functionTypeNode->type) {
        case RBS_TYPES_FUNCTION: {
            function = functionType((rbs_types_function_t *)functionTypeNode, loc, declaration);
            break;
        }
        case RBS_TYPES_UNTYPED_FUNCTION: {
            return function;
        }
        default: {
            auto errLoc = declaration.typeLocFromRange(functionTypeNode->location->rg);
            if (auto e = ctx.beginIndexerError(errLoc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in block type, expected `{}`",
                            rbs_node_type_name(functionTypeNode), "Function");
            }

            return function;
        }
    }

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = declaration.typeLocFromRange(selfNode->location->rg);
        auto selfType = toPrismNode(selfNode, declaration);
        function = prism.Send1(selfLoc, function, "bind"sv, selfType);
    }

    if (!node->required) {
        return prism.TNilable(loc, function);
    }

    return function;
}

pm_node_t *TypeToParserNodePrism::tupleType(const rbs_types_tuple_t *node, core::LocOffsets loc,
                                            const RBSDeclaration &declaration) {
    std::vector<pm_node_t *> typesStore;

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = toPrismNode(list_node->node, declaration);
        typesStore.push_back(innerType);
    }

    return prism.Array(loc, typesStore);
}

pm_node_t *TypeToParserNodePrism::recordType(const rbs_types_record_t *node, core::LocOffsets loc,
                                             const RBSDeclaration &declaration) {
    return prism.ConstantReadNode("Hash"sv, loc);
}

pm_node_t *TypeToParserNodePrism::variableType(const rbs_types_variable_t *node, core::LocOffsets loc) {
    rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node->name;
    auto nameStr = parser.resolveConstant(symbol);
    string nameString(nameStr);
    pm_node_t *symbolNode = prism.Symbol(loc, nameString);
    // fmt::print("DEBUG: Creating type parameter reference: {}\n", nameString);
    return prism.TTypeParameter(loc, symbolNode);
}

pm_node_t *TypeToParserNodePrism::toPrismNode(const rbs_node_t *node, const RBSDeclaration &declaration) {
    auto nodeLoc = declaration.typeLocFromRange(((rbs_node_t *)node)->location->rg);

    // fmt::print("DEBUG type={}\n", rbs_node_type_name((rbs_node_t *)node));
    switch (node->type) {
        case RBS_TYPES_ALIAS:
            return aliasType((rbs_types_alias_t *)node, nodeLoc, declaration);
        case RBS_TYPES_BASES_ANY:
            return prism.TUntyped(nodeLoc);
        case RBS_TYPES_BASES_BOOL:
            return prism.ConstantReadNode("T::Boolean"sv, nodeLoc);
        case RBS_TYPES_BASES_BOTTOM: {
            pm_node_t *t_const = prism.T(nodeLoc);
            if (!t_const) {
                return nullptr;
            }
            return prism.Send0(nodeLoc, t_const, "noreturn"sv);
        }
        case RBS_TYPES_BASES_CLASS: {
            if (auto e = ctx.beginIndexerError(nodeLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS type `{}` is not supported", "class");
            }
            return prism.TUntyped(nodeLoc);
        }
        case RBS_TYPES_BASES_INSTANCE:
            return prism.ConstantReadNode("T.attached_class"sv, nodeLoc);
        case RBS_TYPES_BASES_NIL:
            return prism.ConstantReadNode("NilClass"sv, nodeLoc);
        case RBS_TYPES_BASES_SELF:
            return prism.ConstantReadNode("T.self_type"sv, nodeLoc);
        case RBS_TYPES_BASES_TOP:
            return prism.ConstantReadNode("T.anything"sv, nodeLoc);
        case RBS_TYPES_BASES_VOID:
            return voidType((rbs_types_bases_void_t *)node, nodeLoc);
        case RBS_TYPES_BLOCK:
            return blockType((rbs_types_block_t *)node, nodeLoc, declaration);
        case RBS_TYPES_CLASS_INSTANCE:
            return classInstanceType((rbs_types_class_instance_t *)node, nodeLoc, declaration);
        case RBS_TYPES_CLASS_SINGLETON:
            return classSingletonType((rbs_types_class_singleton_t *)node, nodeLoc, declaration);
        case RBS_TYPES_FUNCTION:
            return functionType((rbs_types_function_t *)node, nodeLoc, declaration);
        case RBS_TYPES_INTERFACE: {
            if (auto e = ctx.beginIndexerError(nodeLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS interfaces are not supported");
            }
            return prism.TUntyped(nodeLoc);
        }
        case RBS_TYPES_INTERSECTION:
            return intersectionType((rbs_types_intersection_t *)node, nodeLoc, declaration);
        case RBS_TYPES_LITERAL: {
            if (auto e = ctx.beginIndexerError(nodeLoc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS literal types are not supported");
            }
            return prism.TUntyped(nodeLoc);
        }
        case RBS_TYPES_OPTIONAL:
            return optionalType((rbs_types_optional_t *)node, nodeLoc, declaration);
        case RBS_TYPES_PROC:
            return procType((rbs_types_proc_t *)node, nodeLoc, declaration);
        case RBS_TYPES_RECORD:
            return recordType((rbs_types_record_t *)node, nodeLoc, declaration);
        case RBS_TYPES_TUPLE:
            return tupleType((rbs_types_tuple_t *)node, nodeLoc, declaration);
        case RBS_TYPES_UNION:
            return unionType((rbs_types_union_t *)node, nodeLoc, declaration);
        case RBS_TYPES_VARIABLE:
            return variableType((rbs_types_variable_t *)node, nodeLoc);
        default: {
            if (auto e = ctx.beginIndexerError(nodeLoc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}`", rbs_node_type_name((rbs_node_t *)node));
            }

            return prism.TUntyped(nodeLoc);
        }
    }
}

} // namespace sorbet::rbs
