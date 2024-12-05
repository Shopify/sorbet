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

core::LocOffsets locOffsets(core::LocOffsets offset, rbs_location_t *loc) {
    return core::LocOffsets{
        offset.beginPos() + loc->rg.start.char_pos + 2,
        offset.beginPos() + loc->rg.end.char_pos + 2,
    };
}

sorbet::ast::ExpressionPtr typeNameType(core::MutableContext ctx, rbs_typename_t *typeName, core::LocOffsets loc) {
    rbs_namespace_t *typeNamespace = typeName->rbs_namespace;
    rbs_node_list *typePath = typeNamespace->path;

    auto parent = ast::MK::EmptyTree();
    if (typePath != nullptr) {
        for (rbs_node_list_node *list_node = typePath->head; list_node != nullptr; list_node = list_node->next) {
            rbs_node_t *node = list_node->node;

            if (node->type != RBS_AST_SYMBOL) {
                std::cout << "unknown node type: " << node->type << std::endl;
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
    rbs_typename_t *typeName = node->name;
    auto offsets = locOffsets(loc, node->location);
    auto typeConstant = typeNameType(ctx, typeName, offsets);

    rbs_node_list *argsValue = node->args;
    if (argsValue != nullptr && argsValue->length > 0) {
        auto argsStore = Send::ARGS_store();
        for (rbs_node_list_node *list_node = argsValue->head; list_node != nullptr; list_node = list_node->next) {
            rbs_node_t *argNode = list_node->node;
            auto argType = TypeTranslator::toRBI(ctx, (rbs_node_t *)argNode, offsets);
            argsStore.emplace_back(std::move(argType));
        }

        return ast::MK::Send(offsets, std::move(typeConstant), core::Names::squareBrackets(), offsets, argsStore.size(),
                             std::move(argsStore));
    }

    return typeConstant;
}

sorbet::ast::ExpressionPtr classSingletonType(core::MutableContext ctx, rbs_types_classsingleton_t *node, core::LocOffsets loc) {
    rbs_typename_t *typeName = node->name;
    auto offsets = locOffsets(loc, node->location);
    auto innerType = typeNameType(ctx, typeName, offsets);

    return ast::MK::ClassOf(offsets, std::move(innerType));
}

sorbet::ast::ExpressionPtr unionType(core::MutableContext ctx, rbs_types_union_t *node, core::LocOffsets loc) {
    rbs_node_list *types = node->types;
    auto typesStore = Send::ARGS_store();
    for (rbs_node_list_node *list_node = types->head; list_node != nullptr; list_node = list_node->next) {
        rbs_node_t *typeNode = list_node->node;
        auto innerType = TypeTranslator::toRBI(ctx, (rbs_node_t *)typeNode, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Any(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr intersectionType(core::MutableContext ctx, rbs_types_intersection_t *node, core::LocOffsets loc) {
    rbs_node_list *types = node->types;
    auto typesStore = Send::ARGS_store();
    for (rbs_node_list_node *list_node = types->head; list_node != nullptr; list_node = list_node->next) {
        rbs_node_t *typeNode = list_node->node;
        auto innerType = TypeTranslator::toRBI(ctx, (rbs_node_t *)typeNode, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::All(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr optionalType(core::MutableContext ctx, rbs_types_optional_t *node, core::LocOffsets loc) {
    rbs_node_t *innerNode = node->type;
    auto innerType = TypeTranslator::toRBI(ctx, innerNode, loc);

    return ast::MK::Nilable(loc, std::move(innerType));
}

sorbet::ast::ExpressionPtr voidType(core::MutableContext ctx, rbs_types_bases_void_t *node, core::LocOffsets loc) {
    auto cSorbet = ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::Sorbet());
    auto cPrivate = ast::MK::UnresolvedConstant(loc, std::move(cSorbet), core::Names::Constants::Private());
    auto cStatic = ast::MK::UnresolvedConstant(loc, std::move(cPrivate), core::Names::Constants::Static());

    return ast::MK::UnresolvedConstant(loc, std::move(cStatic), core::Names::Constants::Void());
}

sorbet::ast::ExpressionPtr functionType(core::MutableContext ctx, rbs_types_function_t *node, core::LocOffsets loc) {
    rbs_node_list *requiredPositionalsValue = node->required_positionals;
    rbs_node_t *returnValue = node->return_type;
    auto returnType = TypeTranslator::toRBI(ctx, returnValue, loc);

    auto paramsStore = Send::ARGS_store();
    int i = 0;
    for (rbs_node_list_node *list_node = requiredPositionalsValue->head; list_node != nullptr; list_node = list_node->next) {
        auto argName = ctx.state.enterNameUTF8("arg" + std::to_string(i));
        paramsStore.emplace_back(ast::MK::Symbol(loc, argName));

        rbs_types_function_param_t *paramNode = (rbs_types_function_param_t *)list_node->node;
        auto innerType = TypeTranslator::toRBI(ctx, paramNode->type, loc);
        paramsStore.emplace_back(std::move(innerType));

        i++;
    }

    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        return ast::MK::T_ProcVoid(loc, std::move(paramsStore));
    }

    return ast::MK::T_Proc(loc, std::move(paramsStore), std::move(returnType));
}

sorbet::ast::ExpressionPtr procType(core::MutableContext ctx, rbs_types_proc_t *node, core::LocOffsets loc) {
    return functionType(ctx, (rbs_types_function_t *)node->type, loc);
}

sorbet::ast::ExpressionPtr blockType(core::MutableContext ctx, rbs_types_block_t *node, core::LocOffsets loc) {
    auto function = functionType(ctx, (rbs_types_function_t *)node->type, loc);

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfType = TypeTranslator::toRBI(ctx, selfNode, loc);
        function = ast::MK::Send1(loc, std::move(function), core::Names::bind(), loc, std::move(selfType));
    }

    if (!node->required) {
        return ast::MK::Nilable(loc, std::move(function));
    }

    return function;
}

sorbet::ast::ExpressionPtr tupleType(core::MutableContext ctx, rbs_types_tuple_t *node, core::LocOffsets loc) {
    rbs_node_list *types = node->types;
    auto typesStore = Array::ENTRY_store();

    for (rbs_node_list_node *list_node = types->head; list_node != nullptr; list_node = list_node->next) {
        rbs_node_t *typeNode = list_node->node;
        auto innerType = TypeTranslator::toRBI(ctx, typeNode, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Array(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr recordType(core::MutableContext ctx, rbs_types_record_t *node, core::LocOffsets loc) {
    rbs_hash_t *entries = node->all_fields;
    auto keysStore = Hash::ENTRY_store();
    auto valuesStore = Hash::ENTRY_store();

    for (rbs_hash_node_t *hash_node = entries->head; hash_node != nullptr; hash_node = hash_node->next) {
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

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, rbs_node_t *node, core::LocOffsets loc) {
    // TODO: handle errors

    switch (node->type) {
        case RBS_TYPES_CLASSINSTANCE:
            return classInstanceType(ctx, (rbs_types_classinstance_t *)node, loc);
        case RBS_TYPES_CLASSSINGLETON:
            return classSingletonType(ctx, (rbs_types_classsingleton_t *)node, loc);
        case RBS_TYPES_OPTIONAL:
            return optionalType(ctx, (rbs_types_optional_t *)node, loc);
        case RBS_TYPES_UNION:
            return unionType(ctx, (rbs_types_union_t *)node, loc);
        case RBS_TYPES_INTERSECTION:
            return intersectionType(ctx, (rbs_types_intersection_t *)node, loc);
        case RBS_TYPES_BASES_SELF:
            return ast::MK::SelfType(loc);
        case RBS_TYPES_BASES_INSTANCE:
            return ast::MK::AttachedClass(loc);
        case RBS_TYPES_BASES_CLASS:
            return ast::MK::Untyped(loc); // TODO: get around type? error
        case RBS_TYPES_BASES_BOOL:
            return ast::MK::T_Boolean(loc);
        case RBS_TYPES_BASES_NIL:
            return ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::NilClass());
        case RBS_TYPES_BASES_ANY:
            return ast::MK::Untyped(loc);
        case RBS_TYPES_BASES_BOTTOM:
            return ast::MK::NoReturn(loc);
        case RBS_TYPES_BASES_TOP:
            return ast::MK::Anything(loc);
        case RBS_TYPES_BASES_VOID:
            return voidType(ctx, (rbs_types_bases_void_t *)node, loc);
        case RBS_TYPES_BLOCK:
            return blockType(ctx, (rbs_types_block_t *)node, loc);
        case RBS_TYPES_PROC:
            return procType(ctx, (rbs_types_proc_t *)node, loc);
        case RBS_TYPES_FUNCTION:
            return functionType(ctx, (rbs_types_function_t *)node, loc);
        case RBS_TYPES_TUPLE:
            return tupleType(ctx, (rbs_types_tuple_t *)node, loc);
        case RBS_TYPES_RECORD:
            return recordType(ctx, (rbs_types_record_t *)node, loc);

        default:
            // TODO: handle errors
            std::cout << "unknown type: " << node->type << std::endl;
            return ast::MK::Untyped(loc);
    }
}

} // namespace sorbet::rbs
