#include "TypeTranslator.h"
#include "ast/Helpers.h"
#include "core/errors/rewriter.h"
#include "core/GlobalState.h"

using namespace sorbet::ast;

namespace sorbet::rbs {

namespace {

bool hasTypeParam(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, std::string_view name) {
    return absl::c_any_of(typeParams, [&](const auto &param) {
        return param.second.toString(ctx.state) == name;
    });
}

sorbet::ast::ExpressionPtr typeNameType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_typename_t *typeName, core::LocOffsets loc) {
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
        } else if (nameStr == "Set") {
            return ast::MK::T_Set(loc);
        } else if (hasTypeParam(ctx, typeParams, nameStr)) {
            auto name = ctx.state.enterNameUTF8(nameStr);
            return ast::MK::Send1(loc, ast::MK::T(loc), core::Names::typeParameter(), loc, ast::MK::Symbol(loc, name));
        }
    }

    auto nameConstant = ctx.state.enterNameConstant(nameStr);

    return ast::MK::UnresolvedConstant(loc, std::move(parent), nameConstant);
}

sorbet::ast::ExpressionPtr classInstanceType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_classinstance_t *node, core::LocOffsets loc) {
    auto offsets = TypeTranslator::nodeLoc(loc, (rbs_node_t *)node);
    auto typeConstant = typeNameType(ctx, typeParams, node->name, offsets);

    rbs_node_list *argsValue = node->args;
    if (argsValue != nullptr && argsValue->length > 0) {
        auto argsStore = Send::ARGS_store();
        for (rbs_node_list_node *list_node = argsValue->head; list_node != nullptr; list_node = list_node->next) {
            auto argType = TypeTranslator::toRBI(ctx, typeParams, list_node->node, loc);
            argsStore.emplace_back(std::move(argType));
        }

        return ast::MK::Send(offsets, std::move(typeConstant), core::Names::squareBrackets(), offsets, argsStore.size(),
                             std::move(argsStore));
    }

    return typeConstant;
}

sorbet::ast::ExpressionPtr classSingletonType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_classsingleton_t *node, core::LocOffsets loc) {
    auto offsets = TypeTranslator::nodeLoc(loc, (rbs_node_t *)node);
    auto innerType = typeNameType(ctx, typeParams, node->name, offsets);

    auto klass = ast::MK::UnresolvedConstantParts(offsets, ast::MK::EmptyTree(),
                                       {core::Names::Constants::T(), core::Names::Constants::Class()});
    return ast::MK::Send1(offsets, std::move(klass), core::Names::squareBrackets(), offsets, std::move(innerType));
}

sorbet::ast::ExpressionPtr unionType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_union_t *node, core::LocOffsets loc) {
    auto typesStore = Send::ARGS_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, typeParams, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Any(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr intersectionType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_intersection_t *node, core::LocOffsets loc) {
    auto typesStore = Send::ARGS_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, typeParams, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::All(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr optionalType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_optional_t *node, core::LocOffsets loc) {
    auto innerType = TypeTranslator::toRBI(ctx, typeParams, node->type, loc);

    return ast::MK::Nilable(loc, std::move(innerType));
}

sorbet::ast::ExpressionPtr voidType(core::MutableContext ctx, rbs_types_bases_void_t *node, core::LocOffsets loc) {
    auto cSorbet = ast::MK::UnresolvedConstant(loc, ast::MK::EmptyTree(), core::Names::Constants::Sorbet());
    auto cPrivate = ast::MK::UnresolvedConstant(loc, std::move(cSorbet), core::Names::Constants::Private());
    auto cStatic = ast::MK::UnresolvedConstant(loc, std::move(cPrivate), core::Names::Constants::Static());

    return ast::MK::UnresolvedConstant(loc, std::move(cStatic), core::Names::Constants::Void());
}

sorbet::ast::ExpressionPtr functionType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_function_t *node, core::LocOffsets loc) {
    std::cout << "functionType" << std::endl;

    auto paramsStore = Send::ARGS_store();
    int i = 0;
    for (rbs_node_list_node *list_node = node->required_positionals->head; list_node != nullptr; list_node = list_node->next) {
        std::cout << "list_node: " << i << std::endl;

        auto argName = ctx.state.enterNameUTF8("arg" + std::to_string(i));
        paramsStore.emplace_back(ast::MK::Symbol(loc, argName));

        std::cout << "list_node: " << list_node << std::endl;
        rbs_node_t *paramNode = list_node->node;
        std::cout << "paramNode: " << paramNode << std::endl;
        sorbet::ast::ExpressionPtr innerType;

        std::cout << "paramNode: " << "here" << std::endl;
        if (paramNode->type != RBS_TYPES_FUNCTION_PARAM) {
            std::cout << "error: " << rbs_node_type_name(paramNode) << std::endl;

            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(paramNode));
            }
            innerType = ast::MK::Untyped(loc);
        } else {
            std::cout << "else: " << rbs_node_type_name(paramNode) << std::endl;

            innerType = TypeTranslator::toRBI(ctx, typeParams, ((rbs_types_function_param_t *)paramNode)->type, loc);
        }

        paramsStore.emplace_back(std::move(innerType));

        i++;
    }

    rbs_node_t *returnValue = node->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        return ast::MK::T_ProcVoid(loc, std::move(paramsStore));
    }

    auto returnType = TypeTranslator::toRBI(ctx, typeParams, returnValue, loc);

    return ast::MK::T_Proc(loc, std::move(paramsStore), std::move(returnType));
}

sorbet::ast::ExpressionPtr procType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_proc_t *node, core::LocOffsets docLoc) {
    auto loc = TypeTranslator::nodeLoc(docLoc, (rbs_node_t *)node);

    rbs_node_t *functionTypeNode = node->type;
    if (functionTypeNode->type != RBS_TYPES_FUNCTION) {
        if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Unexpected node type: {}", rbs_node_type_name(functionTypeNode));
        }
    }

    auto function = functionType(ctx, typeParams, (rbs_types_function_t *)functionTypeNode, loc);

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = TypeTranslator::nodeLoc(loc, selfNode);
        auto selfType = TypeTranslator::toRBI(ctx, typeParams, selfNode, selfLoc);
        function = ast::MK::Send1(loc, std::move(function), core::Names::bind(), loc, std::move(selfType));
    }

    return function;
}

sorbet::ast::ExpressionPtr blockType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_block_t *node, core::LocOffsets docLoc) {
    auto loc = TypeTranslator::nodeLoc(docLoc, (rbs_node_t *)node);

    std::cout << "blockType" << std::endl;

    auto function = ast::MK::Untyped(loc);

    rbs_node_t *functionTypeNode = node->type;
    if (functionTypeNode->type == RBS_TYPES_FUNCTION) {
        function = functionType(ctx, typeParams, (rbs_types_function_t *)functionTypeNode, docLoc);
    // } else if (functionTypeNode->type == RBS_TYPES_UNTYPEDFUNCTION) {
    //     function = untypedFunction(ctx, typeParams, (rbs_types_untypedfunction_t *)functionTypeNode, docLoc);
    } else {
        auto errLoc = TypeTranslator::nodeLoc(docLoc, functionTypeNode);
        if (auto e = ctx.beginError(errLoc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Unexpected node type: {}", rbs_node_type_name(functionTypeNode));
        }

        return function;
    }

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = TypeTranslator::nodeLoc(docLoc, selfNode);
        auto selfType = TypeTranslator::toRBI(ctx, typeParams, selfNode, selfLoc);
        function = ast::MK::Send1(selfLoc, std::move(function), core::Names::bind(), selfLoc, std::move(selfType));
    }

    if (!node->required) {
        return ast::MK::Nilable(loc, std::move(function));
    }

    return function;
}

sorbet::ast::ExpressionPtr tupleType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_tuple_t *node, core::LocOffsets loc) {
    auto typesStore = Array::ENTRY_store();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toRBI(ctx, typeParams, list_node->node, loc);
        typesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Array(loc, std::move(typesStore));
}

sorbet::ast::ExpressionPtr recordType(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams, rbs_types_record_t *node, core::LocOffsets loc) {
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
        keysStore.emplace_back(ast::MK::Symbol(loc, keyName));

        auto innerType = TypeTranslator::toRBI(ctx, typeParams, valueNode->type, loc);
        valuesStore.emplace_back(std::move(innerType));
    }

    return ast::MK::Hash(loc, std::move(keysStore), std::move(valuesStore));
}

sorbet::ast::ExpressionPtr variableType(core::MutableContext ctx, rbs_types_variable_t *node, core::LocOffsets loc) {
    rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node->name;
    rbs_constant_t *constant = rbs_constant_pool_id_to_constant(fake_constant_pool, symbol->constant_id);
    std::string string(constant->start);
    auto name = ctx.state.enterNameUTF8(string);
    return ast::MK::Send1(loc, ast::MK::T(loc), core::Names::typeParameter(), loc, ast::MK::Symbol(loc, name));
}

} // namespace

core::LocOffsets TypeTranslator::nodeLoc(core::LocOffsets offset, rbs_node_t *node) {
    auto loc = node->location;
    return core::LocOffsets{
        offset.beginPos() + loc->rg.start.char_pos + 2,
        offset.beginPos() + loc->rg.end.char_pos + 2,
    };
}

sorbet::ast::ExpressionPtr TypeTranslator::toRBI(core::MutableContext ctx, std::vector<std::pair<core::LocOffsets, core::NameRef>> typeParams,
                                                rbs_node_t *node, core::LocOffsets docLoc) {
    std::cout << "node: " << rbs_node_type_name(node) << std::endl;

    switch (node->type) {
        // TODO: alias?
        case RBS_TYPES_BASES_ANY:
            return ast::MK::Untyped(docLoc);
        case RBS_TYPES_BASES_BOOL:
            return ast::MK::T_Boolean(docLoc);
        case RBS_TYPES_BASES_BOTTOM:
            return ast::MK::NoReturn(docLoc);
        case RBS_TYPES_BASES_CLASS: {
            auto loc = TypeTranslator::nodeLoc(docLoc, node);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
                e.setHeader("RBS type `{}` is not supported yet", "class");
            }
            return ast::MK::Untyped(docLoc);
        }
        case RBS_TYPES_BASES_INSTANCE:
            return ast::MK::AttachedClass(docLoc);
        case RBS_TYPES_BASES_NIL:
            return ast::MK::UnresolvedConstant(docLoc, ast::MK::EmptyTree(), core::Names::Constants::NilClass());
        case RBS_TYPES_BASES_SELF:
            return ast::MK::SelfType(docLoc);
        case RBS_TYPES_BASES_TOP:
            return ast::MK::Anything(docLoc);
        case RBS_TYPES_BASES_VOID:
            return voidType(ctx, (rbs_types_bases_void_t *)node, docLoc);
        case RBS_TYPES_BLOCK:
            return blockType(ctx, typeParams, (rbs_types_block_t *)node, docLoc);
        case RBS_TYPES_CLASSINSTANCE:
            return classInstanceType(ctx, typeParams, (rbs_types_classinstance_t *)node, docLoc);
        case RBS_TYPES_CLASSSINGLETON:
            return classSingletonType(ctx, typeParams, (rbs_types_classsingleton_t *)node, docLoc);
        case RBS_TYPES_FUNCTION:
            return functionType(ctx, typeParams, (rbs_types_function_t *)node, docLoc);
        // TODO: interface?
        case RBS_TYPES_INTERSECTION:
            return intersectionType(ctx, typeParams, (rbs_types_intersection_t *)node, docLoc);
        // TODO: literal?
        case RBS_TYPES_OPTIONAL:
            return optionalType(ctx, typeParams, (rbs_types_optional_t *)node, docLoc);
        case RBS_TYPES_PROC:
            return procType(ctx, typeParams, (rbs_types_proc_t *)node, docLoc);
        case RBS_TYPES_RECORD:
            return recordType(ctx, typeParams, (rbs_types_record_t *)node, docLoc);
        case RBS_TYPES_TUPLE:
            return tupleType(ctx, typeParams, (rbs_types_tuple_t *)node, docLoc);
        case RBS_TYPES_UNION:
            return unionType(ctx, typeParams, (rbs_types_union_t *)node, docLoc);
        // TODO: untyped_function?
        case RBS_TYPES_VARIABLE:
            return variableType(ctx, (rbs_types_variable_t *)node, docLoc);
        default: {
            auto errLoc = TypeTranslator::nodeLoc(docLoc, node);
            if (auto e = ctx.beginError(errLoc, core::errors::Rewriter::RBSError)) {
                e.setHeader("Unexpected node type: {}", rbs_node_type_name(node));
            }

            return ast::MK::Untyped(docLoc);
        }
    }
}

} // namespace sorbet::rbs
