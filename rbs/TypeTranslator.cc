#include "TypeTranslator.h"
#include "ast/Helpers.h"
#include "core/errors/rewriter.h"
#include "core/GlobalState.h"

using namespace sorbet::ast;

namespace sorbet::rbs {

namespace {

sorbet::ast::ExpressionPtr typeNameType(core::MutableContext ctx, rbs_typename_t *typeName, core::LocOffsets loc) {
    rbs_node_list *typePath = typeName->rbs_namespace->path;

    auto parent = ast::MK::EmptyTree();
    if (typePath != nullptr) {
        for (rbs_node_list_node *list_node = typePath->head; list_node != nullptr; list_node = list_node->next) {
            rbs_node_t *node = list_node->node;

            if (node->type != RBS_AST_SYMBOL) {
                if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                    e.setHeader("Unexpected node type: {}", rbs_node_type_name(node));
                }
                continue;
            }

            rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node;
            rbs_constant_t *name = rbs_constant_pool_id_to_constant(fake_constant_pool, symbol->constant_id);
            std::string pathNameStr(name->start);
            auto pathNameConst = ctx.state.enterNameConstant(pathNameStr);
            parent = ast::MK::UnresolvedConstant(loc, std::move(parent), pathNameConst);


        }
    }

    rbs_constant_t *name = rbs_constant_pool_id_to_constant(fake_constant_pool, typeName->name->constant_id);
    std::string nameStr(name->start);

    if (typePath == nullptr || typePath->length == 0) {
        if (nameStr == "Array") {
            return ast::MK::T_Array(loc);
        } else if (nameStr == "Hash") {
            return ast::MK::T_Hash(loc);
        }
    }

    auto nameConstant = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, std::move(parent), nameConstant);
}

sorbet::ast::ExpressionPtr classInstanceType(core::MutableContext ctx, rbs_types_classinstance_t *node, core::LocOffsets loc) {
    auto offsets = TypeTranslator::locOffsets(loc, node->location);
    auto typeConstant = typeNameType(ctx, node->name, offsets);

    rbs_node_list *argsValue = node->args;
    if (argsValue != nullptr && argsValue->length > 0) {
        auto argsStore = Send::ARGS_store();
        for (rbs_node_list_node *list_node = argsValue->head; list_node != nullptr; list_node = list_node->next) {
            auto argType = TypeTranslator::toRBI(ctx, list_node->node, loc);
            argsStore.emplace_back(std::move(argType));
        }

        return ast::MK::Send(offsets, std::move(typeConstant), core::Names::squareBrackets(), offsets, argsStore.size(),
                             std::move(argsStore));
    }

    return typeConstant;
}

sorbet::ast::ExpressionPtr classSingletonType(core::MutableContext ctx, rbs_types_classsingleton_t *node, core::LocOffsets loc) {
    auto offsets = TypeTranslator::locOffsets(loc, node->location);
    auto innerType = typeNameType(ctx, node->name, offsets);

    return ast::MK::ClassOf(offsets, std::move(innerType));
}

sorbet::ast::ExpressionPtr unionType(core::MutableContext ctx, rbs_types_union_t *node, core::LocOffsets loc) {
    auto typesStore = Send::ARGS_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Any(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr intersectionType(core::MutableContext ctx, rbs_types_intersection_t *node, core::LocOffsets loc) {
    auto typesStore = Send::ARGS_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::All(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr optionalType(core::MutableContext ctx, rbs_types_optional_t *node, core::LocOffsets loc) {
    auto innerType = TypeTranslator::toRBI(ctx, node->type, loc);

    return ast::MK::Nilable(loc, std::move(innerType));
}

sorbet::ast::ExpressionPtr voidType(core::MutableContext ctx, rbs_types_bases_void_t *node, core::LocOffsets loc) {
    auto cSorbet = ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::Sorbet());
    auto cPrivate = ast::MK::UnresolvedConstant(loc, std::move(cSorbet), core::Names::Constants::Private());
    auto cStatic = ast::MK::UnresolvedConstant(loc, std::move(cPrivate), core::Names::Constants::Static());

    return ast::MK::UnresolvedConstant(loc, std::move(cStatic), core::Names::Constants::Void());
}

sorbet::ast::ExpressionPtr functionType(core::MutableContext ctx, rbs_types_function_t *node, core::LocOffsets loc) {
    auto paramsStore = Send::ARGS_store();
    int i = 0;
    for (rbs_node_list_node *list_node = node->required_positionals->head; list_node != nullptr; list_node = list_node->next) {
        auto argName = ctx.state.enterNameUTF8("arg" + std::to_string(i));
        paramsStore.emplace_back(ast::MK::Symbol(loc, argName));

        rbs_node_t *paramNode = list_node->node;
        sorbet::ast::ExpressionPtr innerType;

        if (paramNode->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(paramNode));
            }
            innerType = ast::MK::Untyped(loc);
        } else {
            innerType = TypeTranslator::toRBI(ctx, ((rbs_types_function_param_t *)paramNode)->type, loc);
        }

        paramsStore.emplace_back(std::move(innerType));

        i++;
    }

    rbs_node_t *returnValue = node->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        return ast::MK::T_ProcVoid(loc, std::move(paramsStore));
    }

    auto returnType = TypeTranslator::toRBI(ctx, returnValue, loc);

    return ast::MK::T_Proc(loc, std::move(paramsStore), std::move(returnType));
}

sorbet::ast::ExpressionPtr procType(core::MutableContext ctx, rbs_types_proc_t *node, core::LocOffsets loc) {
    rbs_node_t *functionTypeNode = node->type;
    if (functionTypeNode->type != RBS_TYPES_FUNCTION) {
        if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Unexpected node type: {}", rbs_node_type_name(functionTypeNode));
        }
    }

    auto function = functionType(ctx, (rbs_types_function_t *)functionTypeNode, loc);

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = TypeTranslator::nodeLoc(ctx, loc, (rbs_node_t *)selfNode);
        auto selfType = TypeTranslator::toRBI(ctx, selfNode, selfLoc);
        function = ast::MK::Send1(loc, std::move(function), core::Names::bind(), loc, std::move(selfType));
    }

    return function;
}

sorbet::ast::ExpressionPtr blockType(core::MutableContext ctx, rbs_types_block_t *node, core::LocOffsets loc) {
    rbs_node_t *functionTypeNode = node->type;
    if (functionTypeNode->type != RBS_TYPES_FUNCTION) {
        if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Unexpected node type: {}", rbs_node_type_name(functionTypeNode));
        }
    }

    auto function = functionType(ctx, (rbs_types_function_t *)functionTypeNode, loc);

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = TypeTranslator::nodeLoc(ctx, loc, (rbs_node_t *)selfNode);
        auto selfType = TypeTranslator::toRBI(ctx, selfNode, selfLoc);
        function = ast::MK::Send1(selfLoc, std::move(function), core::Names::bind(), selfLoc, std::move(selfType));
    }

    if (!node->required) {
        return ast::MK::Nilable(loc, std::move(function));
    }

    return function;
}

sorbet::ast::ExpressionPtr tupleType(core::MutableContext ctx, rbs_types_tuple_t *node, core::LocOffsets loc) {
    auto typesStore = Array::ENTRY_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Array(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr recordType(core::MutableContext ctx, rbs_types_record_t *node, core::LocOffsets loc) {
    auto keysStore = Hash::ENTRY_store();
    auto valuesStore = Hash::ENTRY_store();

    for (rbs_hash_node_t *hash_node = node->all_fields->head; hash_node != nullptr; hash_node = hash_node->next) {
        if (hash_node->key->type != RBS_AST_SYMBOL) {
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(hash_node->key));
            }

            continue;
        }

        if (hash_node->value->type != RBS_TYPES_RECORD_FIELDTYPE) {
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(hash_node->value));
            }

            continue;
        }

        rbs_ast_symbol_t *keyNode = (rbs_ast_symbol_t *)hash_node->key;
        rbs_types_record_fieldtype_t *valueNode = (rbs_types_record_fieldtype_t *)hash_node->value;

        rbs_constant_t *keyString = rbs_constant_pool_id_to_constant(fake_constant_pool, keyNode->constant_id);
        std::string keyStr(keyString->start);
        auto keyName = ctx.state.enterNameUTF8(keyStr);
        auto keyLiteral =
            ast::MK::Literal(loc, core::make_type<core::NamedLiteralType>(core::Symbols::Symbol(), keyName));
        keysStore.emplace_back(std::move(keyLiteral));

        auto innerType = TypeTranslator::toRBI(ctx, valueNode->type, loc);
        valuesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Hash(loc, std::move(keysStore), std::move(valuesStore));
}

} // namespace

core::LocOffsets TypeTranslator::locOffsets(core::LocOffsets offset, rbs_location_t *loc) {
    return core::LocOffsets{
        offset.beginPos() + loc->rg.start.char_pos + 2,
        offset.beginPos() + loc->rg.end.char_pos + 2,
    };
}

core::LocOffsets TypeTranslator::nodeLoc(core::MutableContext ctx, core::LocOffsets offset, rbs_node_t *node) {
    // Sadly, not all RBS nodes have a location so we have to get it depending on the node type
    switch (node->type) {
        case RBS_AST_ANNOTATION:
            return locOffsets(offset, ((rbs_ast_annotation_t *)node)->location);
        case RBS_AST_COMMENT:
            return locOffsets(offset, ((rbs_ast_comment_t *)node)->location);
        case RBS_AST_DECLARATIONS_CLASS:
            return locOffsets(offset, ((rbs_ast_declarations_class_t *)node)->location);
        case RBS_AST_DECLARATIONS_CLASS_SUPER:
            return locOffsets(offset, ((rbs_ast_declarations_class_super_t *)node)->location);
        case RBS_AST_DECLARATIONS_CLASSALIAS:
            return locOffsets(offset, ((rbs_ast_declarations_classalias_t *)node)->location);
        case RBS_AST_DECLARATIONS_CONSTANT:
            return locOffsets(offset, ((rbs_ast_declarations_constant_t *)node)->location);
        case RBS_AST_DECLARATIONS_GLOBAL:
            return locOffsets(offset, ((rbs_ast_declarations_global_t *)node)->location);
        case RBS_AST_DECLARATIONS_INTERFACE:
            return locOffsets(offset, ((rbs_ast_declarations_interface_t *)node)->location);
        case RBS_AST_DECLARATIONS_MODULE:
            return locOffsets(offset, ((rbs_ast_declarations_module_t *)node)->location);
        case RBS_AST_DECLARATIONS_MODULE_SELF:
            return locOffsets(offset, ((rbs_ast_declarations_module_self_t *)node)->location);
        case RBS_AST_DECLARATIONS_MODULEALIAS:
            return locOffsets(offset, ((rbs_ast_declarations_modulealias_t *)node)->location);
        case RBS_AST_DECLARATIONS_TYPEALIAS:
            return locOffsets(offset, ((rbs_ast_declarations_typealias_t *)node)->location);
        case RBS_AST_DIRECTIVES_USE:
            return locOffsets(offset, ((rbs_ast_directives_use_t *)node)->location);
        case RBS_AST_DIRECTIVES_USE_SINGLECLAUSE:
            return locOffsets(offset, ((rbs_ast_directives_use_singleclause_t *)node)->location);
        case RBS_AST_DIRECTIVES_USE_WILDCARDCLAUSE:
            return locOffsets(offset, ((rbs_ast_directives_use_wildcardclause_t *)node)->location);
        case RBS_AST_MEMBERS_ALIAS:
            return locOffsets(offset, ((rbs_ast_members_alias_t *)node)->location);
        case RBS_AST_MEMBERS_ATTRACCESSOR:
            return locOffsets(offset, ((rbs_ast_members_attraccessor_t *)node)->location);
        case RBS_AST_MEMBERS_ATTRREADER:
            return locOffsets(offset, ((rbs_ast_members_attrreader_t *)node)->location);
        case RBS_AST_MEMBERS_ATTRWRITER:
            return locOffsets(offset, ((rbs_ast_members_attrwriter_t *)node)->location);
        case RBS_AST_MEMBERS_CLASSINSTANCEVARIABLE:
            return locOffsets(offset, ((rbs_ast_members_classinstancevariable_t *)node)->location);
        case RBS_AST_MEMBERS_CLASSVARIABLE:
            return locOffsets(offset, ((rbs_ast_members_classvariable_t *)node)->location);
        case RBS_AST_MEMBERS_EXTEND:
            return locOffsets(offset, ((rbs_ast_members_extend_t *)node)->location);
        case RBS_AST_MEMBERS_INCLUDE:
            return locOffsets(offset, ((rbs_ast_members_include_t *)node)->location);
        case RBS_AST_MEMBERS_INSTANCEVARIABLE:
            return locOffsets(offset, ((rbs_ast_members_instancevariable_t *)node)->location);
        case RBS_AST_MEMBERS_METHODDEFINITION:
            return locOffsets(offset, ((rbs_ast_members_methoddefinition_t *)node)->location);
        case RBS_AST_MEMBERS_PREPEND:
            return locOffsets(offset, ((rbs_ast_members_prepend_t *)node)->location);
        case RBS_AST_MEMBERS_PRIVATE:
            return locOffsets(offset, ((rbs_ast_members_private_t *)node)->location);
        case RBS_AST_MEMBERS_PUBLIC:
            return locOffsets(offset, ((rbs_ast_members_public_t *)node)->location);
        case RBS_AST_TYPEPARAM:
            return locOffsets(offset, ((rbs_ast_typeparam_t *)node)->location);
        case RBS_METHODTYPE:
            return locOffsets(offset, ((rbs_methodtype_t *)node)->location);
        case RBS_TYPES_ALIAS:
            return locOffsets(offset, ((rbs_types_alias_t *)node)->location);
        case RBS_TYPES_BASES_ANY:
            return locOffsets(offset, ((rbs_types_bases_any_t *)node)->location);
        case RBS_TYPES_BASES_BOOL:
            return locOffsets(offset, ((rbs_types_bases_bool_t *)node)->location);
        case RBS_TYPES_BASES_BOTTOM:
            return locOffsets(offset, ((rbs_types_bases_bottom_t *)node)->location);
        case RBS_TYPES_BASES_CLASS:
            return locOffsets(offset, ((rbs_types_bases_class_t *)node)->location);
        case RBS_TYPES_BASES_INSTANCE:
            return locOffsets(offset, ((rbs_types_bases_instance_t *)node)->location);
        case RBS_TYPES_BASES_NIL:
            return locOffsets(offset, ((rbs_types_bases_nil_t *)node)->location);
        case RBS_TYPES_BASES_SELF:
            return locOffsets(offset, ((rbs_types_bases_self_t *)node)->location);
        case RBS_TYPES_BASES_TOP:
            return locOffsets(offset, ((rbs_types_bases_top_t *)node)->location);
        case RBS_TYPES_BASES_VOID:
            return locOffsets(offset, ((rbs_types_bases_void_t *)node)->location);
        case RBS_TYPES_CLASSINSTANCE:
            return locOffsets(offset, ((rbs_types_classinstance_t *)node)->location);
        case RBS_TYPES_CLASSSINGLETON:
            return locOffsets(offset, ((rbs_types_classsingleton_t *)node)->location);
        case RBS_TYPES_FUNCTION_PARAM:
            return locOffsets(offset, ((rbs_types_function_param_t *)node)->location);
        case RBS_TYPES_INTERFACE:
            return locOffsets(offset, ((rbs_types_interface_t *)node)->location);
        case RBS_TYPES_INTERSECTION:
            return locOffsets(offset, ((rbs_types_intersection_t *)node)->location);
        case RBS_TYPES_LITERAL:
            return locOffsets(offset, ((rbs_types_literal_t *)node)->location);
        case RBS_TYPES_OPTIONAL:
            return locOffsets(offset, ((rbs_types_optional_t *)node)->location);
        case RBS_TYPES_PROC:
            return locOffsets(offset, ((rbs_types_proc_t *)node)->location);
        case RBS_TYPES_RECORD:
            return locOffsets(offset, ((rbs_types_record_t *)node)->location);
        case RBS_TYPES_TUPLE:
            return locOffsets(offset, ((rbs_types_tuple_t *)node)->location);
        case RBS_TYPES_UNION:
            return locOffsets(offset, ((rbs_types_union_t *)node)->location);
        case RBS_TYPES_VARIABLE:
            return locOffsets(offset, ((rbs_types_variable_t *)node)->location);
        default: {
            if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
                e.setHeader("No location for RBS node type: {}", rbs_node_type_name(node));
            }

            return offset;
        }
    }
}

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, rbs_node_t *node, core::LocOffsets docLoc) {
    switch (node->type) {
        case RBS_TYPES_CLASSINSTANCE:
            return classInstanceType(ctx, (rbs_types_classinstance_t *)node, docLoc);
        case RBS_TYPES_CLASSSINGLETON:
            return classSingletonType(ctx, (rbs_types_classsingleton_t *)node, docLoc);
        case RBS_TYPES_OPTIONAL:
            return optionalType(ctx, (rbs_types_optional_t *)node, docLoc);
        case RBS_TYPES_UNION:
            return unionType(ctx, (rbs_types_union_t *)node, docLoc);
        case RBS_TYPES_INTERSECTION:
            return intersectionType(ctx, (rbs_types_intersection_t *)node, docLoc);
        case RBS_TYPES_BASES_SELF:
            return ast::MK::SelfType(docLoc);
        case RBS_TYPES_BASES_INSTANCE:
            return ast::MK::AttachedClass(docLoc);
        case RBS_TYPES_BASES_CLASS: {
            auto loc = TypeTranslator::nodeLoc(ctx, docLoc, node);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("RBS type `{}` is not supported yet", "class");
            }
            return ast::MK::Untyped(docLoc);
        }
        case RBS_TYPES_BASES_BOOL:
            return ast::MK::T_Boolean(docLoc);
        case RBS_TYPES_BASES_NIL:
            return ast::MK::UnresolvedConstant(docLoc, ast::MK::EmptyTree(), core::Names::Constants::NilClass());
        case RBS_TYPES_BASES_ANY:
            return ast::MK::Untyped(docLoc);
        case RBS_TYPES_BASES_BOTTOM:
            return ast::MK::NoReturn(docLoc);
        case RBS_TYPES_BASES_TOP:
            return ast::MK::Anything(docLoc);
        case RBS_TYPES_BASES_VOID:
            return voidType(ctx, (rbs_types_bases_void_t *)node, docLoc);
        case RBS_TYPES_BLOCK:
            return blockType(ctx, (rbs_types_block_t *)node, docLoc);
        case RBS_TYPES_PROC:
            return procType(ctx, (rbs_types_proc_t *)node, docLoc);
        case RBS_TYPES_FUNCTION:
            return functionType(ctx, (rbs_types_function_t *)node, docLoc);
        case RBS_TYPES_TUPLE:
            return tupleType(ctx, (rbs_types_tuple_t *)node, docLoc);
        case RBS_TYPES_RECORD:
            return recordType(ctx, (rbs_types_record_t *)node, docLoc);
        default: {
            auto errLoc = TypeTranslator::nodeLoc(ctx, docLoc, node);
            if (auto e = ctx.beginError(errLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unknown RBS node type: {}", rbs_node_type_name(node));
            }

            return ast::MK::Untyped(docLoc);
        }
    }
}

} // namespace sorbet::rbs
