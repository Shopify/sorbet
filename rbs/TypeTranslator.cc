#include "rbs/TypeTranslator.h"
#include "ast/Helpers.h"
#include "core/GlobalState.h"
#include "core/errors/internal.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"

using namespace std;

namespace sorbet::rbs {

namespace {

bool hasTypeParam(core::MutableContext ctx, const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                  core::NameRef name) {
    return absl::c_any_of(typeParams, [&](const auto &param) { return param.second == name; });
}

unique_ptr<parser::Node> typeNameNode(core::MutableContext ctx,
                                      const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                      rbs_typename_t *typeName, bool isGeneric, core::LocOffsets loc) {
    rbs_node_list *typePath = typeName->rbs_namespace->path;

    unique_ptr<parser::Node> parent;
    vector<core::NameRef> pathNames;

    if (typeName->rbs_namespace->absolute) {
        parent = parser::MK::Cbase(loc);
    } else {
        parent = nullptr;
    }

    if (typePath != nullptr) {
        for (rbs_node_list_node *list_node = typePath->head; list_node != nullptr; list_node = list_node->next) {
            rbs_node_t *node = list_node->node;

            ENFORCE(node->type == RBS_AST_SYMBOL, "Unexpected node type `{}` in type name, expected `{}`",
                    rbs_node_type_name(node), "Symbol");

            rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node;
            rbs_constant_t *name = rbs_constant_pool_id_to_constant(fake_constant_pool, symbol->constant_id);
            string pathNameStr(name->start, name->length);
            auto pathNameConst = ctx.state.enterNameConstant(pathNameStr);
            pathNames.emplace_back(pathNameConst);
            parent = parser::MK::Const(loc, move(parent), pathNameConst);
        }
    }

    rbs_constant_t *name = rbs_constant_pool_id_to_constant(fake_constant_pool, typeName->name->constant_id);
    string_view nameStr(name->start, name->length);
    auto nameUTF8 = ctx.state.enterNameUTF8(nameStr);
    auto nameConstant = ctx.state.enterNameConstant(nameUTF8);
    pathNames.emplace_back(nameConstant);

    if (pathNames.size() == 1) {
        if (isGeneric) {
            if (nameConstant == core::Names::Constants::Array()) {
                return parser::MK::T_Array(loc);
            } else if (nameConstant == core::Names::Constants::Class()) {
                return parser::MK::T_Class(loc);
            } else if (nameConstant == core::Names::Constants::Enumerable()) {
                return parser::MK::T_Enumerable(loc);
            } else if (nameConstant == core::Names::Constants::Enumerator()) {
                return parser::MK::T_Enumerator(loc);
            } else if (nameConstant == core::Names::Constants::Hash()) {
                return parser::MK::T_Hash(loc);
            } else if (nameConstant == core::Names::Constants::Set()) {
                return parser::MK::T_Set(loc);
            } else if (nameConstant == core::Names::Constants::Range()) {
                return parser::MK::T_Range(loc);
            }
        } else {
            // The type may refer to a type parameter, so we need to check if it exists as a NameKind::UTF8
            if (hasTypeParam(ctx, typeParams, nameUTF8)) {
                return parser::MK::TTypeParameter(loc, parser::MK::Symbol(loc, nameUTF8));
            }
        }
    } else if (pathNames.size() == 2 && isGeneric) {
        if (pathNames[0] == core::Names::Constants::Enumerator()) {
            if (pathNames[1] == core::Names::Constants::Lazy()) {
                return parser::MK::T_Enumerator_Lazy(loc);
            } else if (pathNames[1] == core::Names::Constants::Chain()) {
                return parser::MK::T_Enumerator_Chain(loc);
            }
        }
    }

    return parser::MK::Const(loc, move(parent), nameConstant);
}

unique_ptr<parser::Node> classInstanceTypeNode(core::MutableContext ctx,
                                               const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                               rbs_types_classinstance_t *node, core::LocOffsets loc) {
    auto offsets = locFromRange(loc, ((rbs_node_t *)node)->location->rg);
    auto argsValue = node->args;
    auto isGeneric = argsValue != nullptr && argsValue->length > 0;
    auto typeConstant = typeNameNode(ctx, typeParams, node->name, isGeneric, offsets);

    if (isGeneric) {
        auto args = parser::NodeVec();
        args.reserve(argsValue->length);
        for (rbs_node_list_node *list_node = argsValue->head; list_node != nullptr; list_node = list_node->next) {
            auto argType = TypeTranslator::toParserNode(ctx, typeParams, list_node->node, loc);
            args.emplace_back(move(argType));
        }

        return parser::MK::Send(offsets, move(typeConstant), core::Names::squareBrackets(), offsets, move(args));
    }

    return typeConstant;
}

unique_ptr<parser::Node> classSingletonTypeNode(core::MutableContext ctx,
                                                const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                                rbs_types_classsingleton_t *node, core::LocOffsets loc) {
    auto offsets = locFromRange(loc, ((rbs_node_t *)node)->location->rg);
    auto innerType = typeNameNode(ctx, typeParams, node->name, false, offsets);
    return parser::MK::TClassOf(loc, move(innerType));
}

unique_ptr<parser::Node> unionTypeNode(core::MutableContext ctx,
                                       const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                       rbs_types_union_t *node, core::LocOffsets loc) {
    auto args = parser::NodeVec();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toParserNode(ctx, typeParams, list_node->node, loc);
        args.emplace_back(move(innerType));
    }

    return parser::MK::TAny(loc, move(args));
}

unique_ptr<parser::Node> intersectionTypeNode(core::MutableContext ctx,
                                              const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                              rbs_types_intersection_t *node, core::LocOffsets loc) {
    auto args = parser::NodeVec();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toParserNode(ctx, typeParams, list_node->node, loc);
        args.emplace_back(move(innerType));
    }

    return parser::MK::TAll(loc, move(args));
}

unique_ptr<parser::Node> optionalTypeNode(core::MutableContext ctx,
                                          const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                          rbs_types_optional_t *node, core::LocOffsets loc) {
    auto innerType = TypeTranslator::toParserNode(ctx, typeParams, node->type, loc);

    if (parser::MK::isTUntyped(innerType)) {
        return innerType;
    }

    return parser::MK::TNilable(loc, move(innerType));
}

unique_ptr<parser::Node> voidTypeNode(core::MutableContext ctx, rbs_types_bases_void_t *node, core::LocOffsets loc) {
    auto cSorbet = parser::MK::Const(loc, parser::MK::Cbase(loc), core::Names::Constants::Sorbet());
    auto cPrivate = parser::MK::Const(loc, move(cSorbet), core::Names::Constants::Private());
    auto cStatic = parser::MK::Const(loc, move(cPrivate), core::Names::Constants::Static());

    return parser::MK::Const(loc, move(cStatic), core::Names::Constants::Void());
}

unique_ptr<parser::Node> functionTypeNode(core::MutableContext ctx,
                                          const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                          rbs_types_function_t *node, core::LocOffsets loc) {
    auto pairs = parser::NodeVec();
    int i = 0;
    for (rbs_node_list_node *list_node = node->required_positionals->head; list_node != nullptr;
         list_node = list_node->next) {
        auto argName = ctx.state.enterNameUTF8("arg" + to_string(i));
        auto key = parser::MK::Symbol(loc, argName);

        rbs_node_t *paramNode = list_node->node;
        unique_ptr<parser::Node> innerType;

        if (paramNode->type != RBS_TYPES_FUNCTION_PARAM) {
            if (auto e = ctx.beginError(loc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in function parameter type, expected `{}`",
                            rbs_node_type_name(paramNode), "FunctionParam");
            }
            innerType = parser::MK::TUntyped(loc);
        } else {
            innerType =
                TypeTranslator::toParserNode(ctx, typeParams, ((rbs_types_function_param_t *)paramNode)->type, loc);
        }

        pairs.emplace_back(make_unique<parser::Pair>(loc, move(key), move(innerType)));

        i++;
    }

    rbs_node_t *returnValue = node->return_type;
    if (returnValue->type == RBS_TYPES_BASES_VOID) {
        return parser::MK::TProcVoid(loc, make_unique<parser::Hash>(loc, true, move(pairs)));
    }

    auto returnType = TypeTranslator::toParserNode(ctx, typeParams, returnValue, loc);

    return parser::MK::TProc(loc, make_unique<parser::Hash>(loc, true, move(pairs)), move(returnType));
}

unique_ptr<parser::Node> procTypeNode(core::MutableContext ctx,
                                      const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                      rbs_types_proc_t *node, core::LocOffsets docLoc) {
    auto loc = locFromRange(docLoc, ((rbs_node_t *)node)->location->rg);
    auto function = parser::MK::TUntyped(loc);

    rbs_node_t *functionType = node->type;
    switch (functionType->type) {
        case RBS_TYPES_FUNCTION: {
            function = functionTypeNode(ctx, typeParams, (rbs_types_function_t *)functionType, loc);
            break;
        }
        case RBS_TYPES_UNTYPEDFUNCTION: {
            return function;
        }
        default: {
            auto errLoc = locFromRange(docLoc, functionType->location->rg);
            if (auto e = ctx.beginError(errLoc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in proc type, expected `{}`", rbs_node_type_name(functionType),
                            "Function");
            }
        }
    }

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = locFromRange(loc, selfNode->location->rg);
        auto selfType = TypeTranslator::toParserNode(ctx, typeParams, selfNode, selfLoc);
        function = parser::MK::Send1(loc, move(function), core::Names::bind(), loc, move(selfType));
    }

    return function;
}

unique_ptr<parser::Node> blockTypeNode(core::MutableContext ctx,
                                       const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                       rbs_types_block_t *node, core::LocOffsets docLoc) {
    auto loc = locFromRange(docLoc, ((rbs_node_t *)node)->location->rg);
    auto function = parser::MK::TUntyped(loc);

    rbs_node_t *functionType = node->type;
    switch (functionType->type) {
        case RBS_TYPES_FUNCTION: {
            function = functionTypeNode(ctx, typeParams, (rbs_types_function_t *)functionType, docLoc);
            break;
        }
        case RBS_TYPES_UNTYPEDFUNCTION: {
            return function;
        }
        default: {
            auto errLoc = locFromRange(docLoc, functionType->location->rg);
            if (auto e = ctx.beginError(errLoc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in block type, expected `{}`", rbs_node_type_name(functionType),
                            "Function");
            }

            return function;
        }
    }

    rbs_node_t *selfNode = node->self_type;
    if (selfNode != nullptr) {
        auto selfLoc = locFromRange(docLoc, selfNode->location->rg);
        auto selfType = TypeTranslator::toParserNode(ctx, typeParams, selfNode, selfLoc);
        function = parser::MK::Send1(selfLoc, move(function), core::Names::bind(), selfLoc, move(selfType));
    }

    if (!node->required) {
        return parser::MK::TNilable(loc, move(function));
    }

    return function;
}

unique_ptr<parser::Node> tupleTypeNode(core::MutableContext ctx,
                                       const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                       rbs_types_tuple_t *node, core::LocOffsets loc) {
    auto args = parser::NodeVec();

    for (rbs_node_list_node *list_node = node->types->head; list_node != nullptr; list_node = list_node->next) {
        auto innerType = TypeTranslator::toParserNode(ctx, typeParams, list_node->node, loc);
        args.emplace_back(move(innerType));
    }

    return parser::MK::Array(loc, move(args));
}

unique_ptr<parser::Node> recordTypeNode(core::MutableContext ctx,
                                        const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                        rbs_types_record_t *node, core::LocOffsets loc) {
    auto pairs = parser::NodeVec();

    for (rbs_hash_node_t *hash_node = node->all_fields->head; hash_node != nullptr; hash_node = hash_node->next) {
        unique_ptr<parser::Node> key;

        switch (hash_node->key->type) {
            case RBS_AST_SYMBOL: {
                rbs_ast_symbol_t *keyNode = (rbs_ast_symbol_t *)hash_node->key;
                rbs_constant_t *keyString = rbs_constant_pool_id_to_constant(fake_constant_pool, keyNode->constant_id);
                string_view keyStr(keyString->start, keyString->length);
                auto keyName = ctx.state.enterNameUTF8(keyStr);
                key = parser::MK::Symbol(loc, keyName);
                break;
            }
            case RBS_AST_STRING: {
                rbs_ast_string_t *keyNode = (rbs_ast_string_t *)hash_node->key;
                string_view keyStr(keyNode->string.start);
                auto keyName = ctx.state.enterNameUTF8(keyStr);
                key = parser::MK::String(loc, keyName);
                break;
            }
            default: {
                if (auto e = ctx.beginError(loc, core::errors::Internal::InternalError)) {
                    e.setHeader("Unexpected node type `{}` in record key type, expected `{}`",
                                rbs_node_type_name(hash_node->key), "Symbol");
                }
                continue;
            }
        }

        if (hash_node->value->type != RBS_TYPES_RECORD_FIELDTYPE) {
            if (auto e = ctx.beginError(loc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}` in record value type, expected `{}`",
                            rbs_node_type_name(hash_node->value), "RecordFieldtype");
            }

            continue;
        }

        rbs_types_record_fieldtype_t *valueNode = (rbs_types_record_fieldtype_t *)hash_node->value;
        auto innerType = TypeTranslator::toParserNode(ctx, typeParams, valueNode->type, loc);
        pairs.emplace_back(std::make_unique<parser::Pair>(loc, move(key), move(innerType)));
    }

    return parser::MK::Hash(loc, false, move(pairs));
}

unique_ptr<parser::Node> variableTypeNode(core::MutableContext ctx, rbs_types_variable_t *node, core::LocOffsets loc) {
    rbs_ast_symbol_t *symbol = (rbs_ast_symbol_t *)node->name;
    rbs_constant_t *constant = rbs_constant_pool_id_to_constant(fake_constant_pool, symbol->constant_id);
    string_view str(constant->start, constant->length);
    auto name = ctx.state.enterNameUTF8(str);
    return parser::MK::TTypeParameter(loc, parser::MK::Symbol(loc, name));
}

} // namespace

unique_ptr<parser::Node> TypeTranslator::toParserNode(core::MutableContext ctx,
                                                      const vector<pair<core::LocOffsets, core::NameRef>> &typeParams,
                                                      rbs_node_t *node, core::LocOffsets docLoc) {
    switch (node->type) {
        case RBS_TYPES_ALIAS: {
            auto loc = locFromRange(docLoc, node->location->rg);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS aliases are not supported");
            }
            return parser::MK::TUntyped(docLoc);
        }
        case RBS_TYPES_BASES_ANY:
            return parser::MK::TUntyped(docLoc);
        case RBS_TYPES_BASES_BOOL:
            return parser::MK::T_Boolean(docLoc);
        case RBS_TYPES_BASES_BOTTOM:
            return parser::MK::TNoReturn(docLoc);
        case RBS_TYPES_BASES_CLASS: {
            auto loc = locFromRange(docLoc, node->location->rg);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS type `{}` is not supported", "class");
            }
            return parser::MK::TUntyped(docLoc);
        }
        case RBS_TYPES_BASES_INSTANCE:
            return parser::MK::TAttachedClass(docLoc);
        case RBS_TYPES_BASES_NIL:
            return parser::MK::Const(docLoc, parser::MK::Cbase(docLoc), core::Names::Constants::NilClass());
        case RBS_TYPES_BASES_SELF:
            return parser::MK::TSelfType(docLoc);
        case RBS_TYPES_BASES_TOP:
            return parser::MK::TAnything(docLoc);
        case RBS_TYPES_BASES_VOID:
            return voidTypeNode(ctx, (rbs_types_bases_void_t *)node, docLoc);
        case RBS_TYPES_BLOCK:
            return blockTypeNode(ctx, typeParams, (rbs_types_block_t *)node, docLoc);
        case RBS_TYPES_CLASSINSTANCE:
            return classInstanceTypeNode(ctx, typeParams, (rbs_types_classinstance_t *)node, docLoc);
        case RBS_TYPES_CLASSSINGLETON:
            return classSingletonTypeNode(ctx, typeParams, (rbs_types_classsingleton_t *)node, docLoc);
        case RBS_TYPES_FUNCTION:
            return functionTypeNode(ctx, typeParams, (rbs_types_function_t *)node, docLoc);
        case RBS_TYPES_INTERFACE: {
            auto loc = locFromRange(docLoc, node->location->rg);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS interfaces are not supported");
            }
            return parser::MK::TUntyped(docLoc);
        }
        case RBS_TYPES_INTERSECTION:
            return intersectionTypeNode(ctx, typeParams, (rbs_types_intersection_t *)node, docLoc);
        case RBS_TYPES_LITERAL: {
            auto loc = locFromRange(docLoc, node->location->rg);
            if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("RBS literal types are not supported");
            }
            return parser::MK::TUntyped(docLoc);
        }
        case RBS_TYPES_OPTIONAL:
            return optionalTypeNode(ctx, typeParams, (rbs_types_optional_t *)node, docLoc);
        case RBS_TYPES_PROC:
            return procTypeNode(ctx, typeParams, (rbs_types_proc_t *)node, docLoc);
        case RBS_TYPES_RECORD:
            return recordTypeNode(ctx, typeParams, (rbs_types_record_t *)node, docLoc);
        case RBS_TYPES_TUPLE:
            return tupleTypeNode(ctx, typeParams, (rbs_types_tuple_t *)node, docLoc);
        case RBS_TYPES_UNION:
            return unionTypeNode(ctx, typeParams, (rbs_types_union_t *)node, docLoc);
        case RBS_TYPES_VARIABLE:
            return variableTypeNode(ctx, (rbs_types_variable_t *)node, docLoc);
        default: {
            auto errLoc = locFromRange(docLoc, node->location->rg);
            if (auto e = ctx.beginError(errLoc, core::errors::Internal::InternalError)) {
                e.setHeader("Unexpected node type `{}`", rbs_node_type_name(node));
            }

            return parser::MK::TUntyped(docLoc);
        }
    }
}

} // namespace sorbet::rbs
