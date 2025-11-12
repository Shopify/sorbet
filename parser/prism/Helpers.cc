#include "parser/prism/Helpers.h"
#include "parser/prism/Parser.h"

namespace sorbet::parser::Prism {

using namespace std;
using namespace std::literals::string_view_literals;

pm_node_t **Factory::copyNodesToArray(const vector<pm_node_t *> &nodes) {
    pm_node_t **nodeArray = nullptr;
    try {
        nodeArray = new pm_node_t *[nodes.size()];
        for (size_t i = 0; i < nodes.size(); i++) {
            nodeArray[i] = nodes[i];
        }
    } catch (...) {
        Exception::raise("Failed to copy nodes to array");
    }
    return nodeArray;
}

pm_arguments_node_t *Factory::createArgumentsNode(vector<pm_node_t *> args, const pm_location_t loc) {
    pm_arguments_node_t *arguments = Factory::allocateNode<pm_arguments_node_t>();

    pm_node_t **argNodes = copyNodesToArray(args);

    size_t argsSize = args.size();
    *arguments = (pm_arguments_node_t){.base = initializeBaseNode(PM_ARGUMENTS_NODE, loc),
                                       .arguments = {.size = argsSize, .capacity = argsSize, .nodes = argNodes}};

    return arguments;
}

pm_node_t Factory::initializeBaseNode(pm_node_type_t type, const pm_location_t loc) {
    pm_parser_t *prismParser = parser.getRawParserPointer();
    prismParser->node_id++;
    uint32_t nodeId = prismParser->node_id;

    return (pm_node_t){.type = type, .flags = 0, .node_id = nodeId, .location = loc};
}

pm_node_t *Factory::ConstantReadNode(string_view name, core::LocOffsets loc) {
    pm_constant_id_t constantId = addConstantToPool(name);
    if (constantId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    pm_constant_read_node_t *node = allocateNode<pm_constant_read_node_t>();

    *node = (pm_constant_read_node_t){.base = initializeBaseNode(PM_CONSTANT_READ_NODE, getZeroWidthLocation()),
                                      .name = constantId};

    return up_cast(node);
}

pm_node_t *Factory::ConstantWriteNode(core::LocOffsets loc, pm_constant_id_t nameId, pm_node_t *value) {
    if (!value) {
        return nullptr;
    }

    pm_constant_write_node_t *node = allocateNode<pm_constant_write_node_t>();

    pm_location_t pmLoc = convertLocOffsets(loc);
    pm_location_t zeroLoc = getZeroWidthLocation();

    *node = (pm_constant_write_node_t){.base = initializeBaseNode(PM_CONSTANT_WRITE_NODE, pmLoc),
                                       .name = nameId,
                                       .name_loc = pmLoc,
                                       .value = value,
                                       .operator_loc = zeroLoc};

    return up_cast(node);
}

pm_node_t *Factory::ConstantPathNode(core::LocOffsets loc, pm_node_t *parent, string_view name) {
    pm_constant_id_t nameId = addConstantToPool(name);
    if (nameId == PM_CONSTANT_ID_UNSET) {
        return nullptr;

        pm_constant_path_node_t *node = allocateNode<pm_constant_path_node_t>();

        pm_location_t pmLoc = convertLocOffsets(loc);

        *node = (pm_constant_path_node_t){.base = initializeBaseNode(PM_CONSTANT_PATH_NODE, pmLoc),
                                          .parent = parent,
                                          .name = name_id,
                                          .delimiter_loc = pmLoc,
                                          .name_loc = pmLoc};

        // The translator uses node->location (i.e., node->base.location) to assign locations to
        // the translated Whitequark AST nodes. Without this, nodes get {0,0} locations, breaking
        // features like check-and-and creation.
        node->base.location = pmLoc;

        return up_cast(node);
    }

    pm_node_t *Factory::SingleArgumentNode(pm_node_t * arg) {
        vector<pm_node_t *> args = {arg};
        pm_arguments_node_t *arguments = createArgumentsNode(args, arg->location);

        return up_cast(arguments);
    }

    pm_node_t *Factory::Self(core::LocOffsets loc) {
        pm_self_node_t *selfNode = allocateNode<pm_self_node_t>();

        *selfNode = (pm_self_node_t){.base = initializeBaseNode(PM_SELF_NODE, convertLocOffsets(loc))};

        return up_cast(selfNode);
    }

    pm_constant_id_t Factory::addConstantToPool(string_view name) { // can this be c++ ified
        pm_parser_t *prismParser = parser.getRawParserPointer();
        size_t nameLen = name.size();
        uint8_t *stable = (uint8_t *)calloc(nameLen, sizeof(uint8_t));
        if (!stable) {
            return PM_CONSTANT_ID_UNSET;
        }
        memcpy(stable, name.data(), nameLen);
        pm_constant_id_t id = pm_constant_pool_insert_owned(&prismParser->constant_pool, stable, nameLen);
        return id;
    }

    pm_location_t Factory::getZeroWidthLocation() {
        pm_parser_t *prismParser = parser.getRawParserPointer();
        const uint8_t *sourceStart = prismParser->start;
        return {.start = sourceStart, .end = sourceStart};
    }

    pm_location_t Factory::convertLocOffsets(core::LocOffsets loc) {
        pm_parser_t *prismParser = parser.getRawParserPointer();
        const uint8_t *sourceStart = prismParser->start;

        const uint8_t *start_ptr = source_start + loc.beginPos();
        const uint8_t *end_ptr = source_start + loc.endPos();

        return {.start = start_ptr, .end = end_ptr};
    }

    pm_call_node_t *Factory::createSendNode(pm_node_t * receiver, pm_constant_id_t methodId, pm_node_t * arguments,
                                            pm_location_t messageLoc, pm_location_t fullLoc, pm_location_t tinyLoc,
                                            pm_node_t * block) {
        pm_call_node_t *call = allocateNode<pm_call_node_t>();

        *call = (pm_call_node_t){.base = initializeBaseNode(PM_CALL_NODE, fullLoc),
                                 .receiver = receiver,
                                 .call_operator_loc = tiny_loc,
                                 .name = method_id,
                                 .message_loc = message_loc,
                                 .opening_loc = tiny_loc,
                                 .arguments = down_cast<pm_arguments_node_t>(arguments),
                                 .closing_loc = tiny_loc,
                                 .block = block};

        return call;
    }

    pm_node_t *Factory::SymbolFromConstant(core::LocOffsets nameLoc, pm_constant_id_t nameId) {
        if (nameId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        auto nameView = parser.resolveConstant(nameId);
        size_t nameSize = nameView.size();

        uint8_t *stable = (uint8_t *)calloc(nameString.size(), sizeof(uint8_t));
        if (!stable) {
            return nullptr;
        }
        memcpy(stable, nameString.data(), nameString.size());

        pm_symbol_node_t *symbolNode = allocateNode<pm_symbol_node_t>();
        if (!symbolNode) {
            return nullptr;
        }

        pm_location_t location = convertLocOffsets(nameLoc.copyWithZeroLength());

        pm_string_t unescaped_string;
        unescaped_string.source = stable;
        unescaped_string.length = nameString.length();

        *symbolNode = (pm_symbol_node_t){.base = initializeBaseNode(PM_SYMBOL_NODE, location),
                                         .opening_loc = location,
                                         .value_loc = location,
                                         .closing_loc = location,
                                         .unescaped = unescapedString};

        return up_cast(symbolNode);
    }

    pm_node_t *Factory::AssocNode(core::LocOffsets loc, pm_node_t * key, pm_node_t * value) {
        if (!key || !value) {
            Exception::raise("Key or value is null");
        }

        pm_assoc_node_t *assocNode = allocateNode<pm_assoc_node_t>();

        pm_location_t location = convertLocOffsets(loc.copyWithZeroLength());

        *assocNode = (pm_assoc_node_t){
            .base = initializeBaseNode(PM_ASSOC_NODE, location), .key = key, .value = value, .operator_loc = location};

        return up_cast(assocNode);
    }

    pm_node_t *Factory::Hash(core::LocOffsets loc, const vector<pm_node_t *> &pairs) {
        if (pairs.empty()) {
            return nullptr;
        }

        pm_hash_node_t *hashNode = allocateNode<pm_hash_node_t>();

        pm_node_t **elements = copyNodesToArray(pairs);

        pm_location_t base_loc = convertLocOffsets(loc);
        pm_location_t opening_loc = {.start = nullptr, .end = nullptr};
        pm_location_t closing_loc = {.start = nullptr, .end = nullptr};

        size_t pairsSize = pairs.size();
        *hashNode = (pm_hash_node_t){.base = initializeBaseNode(PM_HASH_NODE, baseLoc),
                                     .opening_loc = openingLoc,
                                     .elements = {.size = pairsSize, .capacity = pairsSize, .nodes = elements},
                                     .closing_loc = closingLoc};

        return up_cast(hashNode);
    }

    pm_node_t *Factory::KeywordHash(core::LocOffsets loc, const vector<pm_node_t *> &pairs) {
        if (pairs.empty()) {
            return nullptr;
        }

        pm_keyword_hash_node_t *hashNode = allocateNode<pm_keyword_hash_node_t>();

        pm_node_t **elements = copyNodesToArray(pairs);

        pm_location_t base_loc = convertLocOffsets(loc);

        size_t pairsSize = pairs.size();
        *hashNode = (pm_keyword_hash_node_t){.base = initializeBaseNode(PM_KEYWORD_HASH_NODE, baseLoc),
                                             .elements = {.size = pairsSize, .capacity = pairsSize, .nodes = elements}};

        return up_cast(hashNode);
    }

    pm_node_t *Factory::SorbetPrivateStatic(core::LocOffsets loc) {
        // Build a root-anchored constant path ::Sorbet::Private::Static
        pm_node_t *sorbet = ConstantPathNode(loc, nullptr, "Sorbet"sv);

        pm_node_t *sorbetPrivate = ConstantPathNode(loc, sorbet, "Private"sv);
        return ConstantPathNode(loc, sorbetPrivate, "Static"sv);
    }

    pm_node_t *Factory::TSigWithoutRuntime(core::LocOffsets loc) {
        // Build a root-anchored constant path ::T::Sig::WithoutRuntime
        pm_node_t *tConst = ConstantPathNode(loc, nullptr, "T"sv);
        pm_node_t *tSig = ConstantPathNode(loc, tConst, "Sig"sv);

        return ConstantPathNode(loc, tSig, "WithoutRuntime"sv);
    }

    pm_node_t *Factory::Symbol(core::LocOffsets nameLoc, string_view name) {
        if (name.empty()) {
            Exception::raise("Name is empty");
        }

        pm_constant_id_t nameId = addConstantToPool(name);
        return SymbolFromConstant(nameLoc, nameId);
    }

    pm_node_t *Factory::Send0(core::LocOffsets loc, pm_node_t * receiver, string_view method) {
        if (!receiver || method.empty()) {
            Exception::raise("Receiver or method is null");
        }

        pm_constant_id_t methodId = addConstantToPool(method);
        if (method_id == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        pm_location_t full_loc = convertLocOffsets(loc);
        pm_location_t tiny_loc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(receiver, method_id, nullptr, tiny_loc, full_loc, tiny_loc));
    }

    pm_node_t *Factory::Send1(core::LocOffsets loc, pm_node_t * receiver, string_view method, pm_node_t * arg1) {
        if (!receiver || method.empty() || !arg1) {
            Exception::raise("Receiver or method is null");
        }

        pm_constant_id_t methodId = addConstantToPool(method);
        if (methodId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        pm_node_t *arguments = SingleArgumentNode(arg1);

        pm_location_t full_loc = convertLocOffsets(loc);
        pm_location_t tiny_loc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(receiver, method_id, arguments, tiny_loc, full_loc, tiny_loc));
    }

    pm_node_t *PMK::Send(core::LocOffsets loc, pm_node_t * receiver, const char *method,
                         const vector<pm_node_t *> &args, pm_node_t *block) {
        if (!receiver || !method) {
            return nullptr;
        }

        pm_constant_id_t methodId = addConstantToPool(method);
        if (methodId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        pm_arguments_node_t *arguments = nullptr;
        if (!args.empty()) {
            arguments = createArgumentsNode(args, loc);
            if (!arguments) {
                return nullptr;
            }
        }

        pm_location_t fullLoc = convertLocOffsets(loc);
        pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(receiver, method_id, up_cast(arguments), tiny_loc, full_loc, tiny_loc, block));
    }

    pm_node_t *Factory::T(core::LocOffsets loc) {
        return ConstantPathNode(loc, nullptr, "T"sv);
    }

    pm_node_t *PMK::TUntyped(core::LocOffsets loc) {
        // Create T.untyped call
        pm_node_t *tConst = T(loc);
        if (!tConst) {
            return nullptr;
        }

        return Send0(loc, tConst, "untyped");
    }

    pm_node_t *PMK::TNilable(core::LocOffsets loc, pm_node_t * type) {
        // Create T.nilable(type) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !type) {
            return nullptr;
        }

        return Send1(loc, tConst, "nilable", type);
    }

    pm_node_t *PMK::TAny(core::LocOffsets loc, const vector<pm_node_t *> &args) {
        // Create T.any(args...) call
        pm_node_t *tConst = T(loc);
        if (!tConst || args.empty()) {
            return nullptr;
        }

        pm_constant_id_t methodId = addConstantToPool("any");
        if (methodId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        pm_arguments_node_t *arguments = createArgumentsNode(args, loc);
        if (!arguments) {
            return nullptr;
        }

        pm_location_t full_loc = convertLocOffsets(loc);
        pm_location_t tiny_loc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(t_const, method_id, up_cast(arguments), tiny_loc, full_loc, tiny_loc));
    }

    pm_node_t *PMK::TAll(core::LocOffsets loc, const vector<pm_node_t *> &args) {
        // Create T.all(args...) call
        pm_node_t *tConst = T(loc);
        if (!tConst || args.empty()) {
            return nullptr;
        }

        pm_constant_id_t methodId = addConstantToPool("all");
        if (methodId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        pm_arguments_node_t *arguments = createArgumentsNode(args, loc);
        if (!arguments) {
            return nullptr;
        }

        pm_location_t full_loc = convertLocOffsets(loc);
        pm_location_t tiny_loc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(t_const, method_id, up_cast(arguments), tiny_loc, full_loc, tiny_loc));
    }

    pm_node_t *PMK::TTypeParameter(core::LocOffsets loc, pm_node_t * name) {
        // Create T.type_parameter(name) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !name) {
            return nullptr;
        }

        return Send1(loc, tConst, "type_parameter", name);
    }

    pm_node_t *Factory::TProc(core::LocOffsets loc, pm_node_t * args, pm_node_t * returnType) {
        pm_node_t *builder = T(loc);
        if (!returnType) {
            Exception::raise("Return type is null");
        }

        builder = Send0(loc, builder, "proc"sv);

        if (args != nullptr) {
            builder = Send1(loc, builder, "params"sv, args);
        }

        return Send1(loc, builder, "returns"sv, returnType);
    }

    pm_node_t *Factory::TProcVoid(core::LocOffsets loc, pm_node_t * args) {
        pm_node_t *builder = T(loc);

        builder = Send0(loc, builder, "proc"sv);

        if (args != nullptr) {
            builder = Send1(loc, builder, "params"sv, args);
        }

        return Send0(loc, builder, "void"sv);
    }

    pm_node_t *Factory::Send2(core::LocOffsets loc, pm_node_t * receiver, string_view method, pm_node_t * arg1,
                              pm_node_t * arg2) {
        if (!receiver || method.empty() || !arg1 || !arg2) {
            Exception::raise("Receiver, method, or arguments are null");
        }

        pm_constant_id_t methodId = addConstantToPool(method);
        if (methodId == PM_CONSTANT_ID_UNSET) {
            return nullptr;
        }

        // Create arguments node with two arguments
        vector<pm_node_t *> args = {arg1, arg2};
        pm_arguments_node_t *arguments = createArgumentsNode(args, loc);
        if (!arguments) {
            return nullptr;
        }

        pm_node_t **arg_nodes = (pm_node_t **)calloc(2, sizeof(pm_node_t *));
        if (!arg_nodes) {
            free(arguments);
            return nullptr;
        }

        arg_nodes[0] = arg1;
        arg_nodes[1] = arg2;

        *arguments = (pm_arguments_node_t){.base = initializeBaseNode(PM_ARGUMENTS_NODE),
                                           .arguments = {.size = 2, .capacity = 2, .nodes = arg_nodes}};
        arguments->base.location = convertLocOffsets(loc);

        pm_location_t full_loc = convertLocOffsets(loc);
        pm_location_t tiny_loc = convertLocOffsets(loc.copyWithZeroLength());

        return up_cast(createSendNode(receiver, method_id, up_cast(arguments), tiny_loc, full_loc, tiny_loc));
    }

    pm_node_t *PMK::TLet(core::LocOffsets loc, pm_node_t * value, pm_node_t * type) {
        // Create T.let(value, type) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !value || !type) {
            return nullptr;
        }

        return Send2(loc, tConst, "let", value, type);
    }

    pm_node_t *PMK::TCast(core::LocOffsets loc, pm_node_t * value, pm_node_t * type) {
        // Create T.cast(value, type) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !value || !type) {
            return nullptr;
        }

        return Send2(loc, tConst, "cast", value, type);
    }

    pm_node_t *PMK::TMust(core::LocOffsets loc, pm_node_t * value) {
        // Create T.must(value) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !value) {
            return nullptr;
        }

        return Send1(loc, tConst, "must", value);
    }

    pm_node_t *PMK::TUnsafe(core::LocOffsets loc, pm_node_t * value) {
        // Create T.unsafe(value) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !value) {
            return nullptr;
        }

        return Send1(loc, tConst, "unsafe", value);
    }

    pm_node_t *PMK::TAbsurd(core::LocOffsets loc, pm_node_t * value) {
        // Create T.absurd(value) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !value) {
            return nullptr;
        }

        return Send1(loc, tConst, "absurd", value);
    }

    pm_node_t *PMK::TBindSelf(core::LocOffsets loc, pm_node_t * type) {
        // Create T.bind(self, type) call
        pm_node_t *tConst = T(loc);
        if (!tConst || !type) {
            return nullptr;
        }

        pm_node_t *selfNode = Self(loc);
        if (!selfNode) {
            return nullptr;
        }

        return Send2(loc, tConst, "bind", selfNode, type);
    }

    pm_node_t *PMK::TTypeAlias(core::LocOffsets loc, pm_node_t * type) {
        pm_node_t *tConst = T(loc);
        if (!tConst || !type) {
            return nullptr;
        }

        pm_node_t *send = Send0(loc, tConst, "type_alias");
        if (!send) {
            return nullptr;
        }

        pm_statements_node_t *stmts = allocateNode<pm_statements_node_t>();
        if (!stmts) {
            return nullptr;
        }
        *stmts = (pm_statements_node_t){.base = initializeBaseNode(PM_STATEMENTS_NODE),
                                        .body = {.size = 0, .capacity = 0, .nodes = nullptr}};
        pm_node_list_append(&stmts->body, type);

        pm_block_node_t *block = allocateNode<pm_block_node_t>();
        if (!block) {
            free(stmts);
            return nullptr;
        }
        *block = (pm_block_node_t){.base = initializeBaseNode(PM_BLOCK_NODE),
                                   .locals = {.size = 0, .capacity = 0, .ids = nullptr},
                                   .parameters = nullptr,
                                   .body = up_cast(stmts),
                                   .opening_loc = getZeroWidthLocation(),
                                   .closing_loc = getZeroWidthLocation()};

        auto *call = down_cast<pm_call_node_t>(send);
        call->block = up_cast(block);

        return send;
    }

    pm_node_t *Factory::Array(core::LocOffsets loc, const vector<pm_node_t *> &elements) {
        pm_array_node_t *array = allocateNode<pm_array_node_t>();

        pm_node_t **elem_nodes = nullptr;
        if (!elements.empty()) {
            elemNodes = copyNodesToArray(elements);
            if (!elemNodes) {
                free(array);
                return nullptr;
            }
        }

        *array = (pm_array_node_t){.base = initializeBaseNode(PM_ARRAY_NODE),
                                   .elements = {.size = elementsSize, .capacity = elementsSize, .nodes = elemNodes},
                                   .opening_loc = convertLocOffsets(loc.copyWithZeroLength()),
                                   .closing_loc = convertLocOffsets(loc.copyEndWithZeroLength())};

        array->base.location = convertLocOffsets(loc);

        return up_cast(array);
    }

    pm_node_t *Factory::T_Array(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Array"sv);
    }

    pm_node_t *Factory::T_Class(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Class"sv);
    }

    pm_node_t *Factory::T_Enumerable(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Enumerable"sv);
    }

    pm_node_t *Factory::T_Enumerator(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Enumerator"sv);
    }

    pm_node_t *Factory::T_Hash(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Hash"sv);
    }

    pm_node_t *Factory::T_Set(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Set"sv);
    }

    pm_node_t *Factory::T_Range(core::LocOffsets loc) {
        return ConstantPathNode(loc, T(loc), "Range"sv);
    }

    bool PMK::isTUntyped(pm_node_t * node) {
        if (!node || PM_NODE_TYPE(node) != PM_CALL_NODE) {
            return false;
        }

        pm_call_node_t *call = down_cast<pm_call_node_t>(node);
        if (!call->receiver || PM_NODE_TYPE(call->receiver) != PM_CONSTANT_PATH_NODE) {
            return false;
        }

        pm_constant_path_node_t *receiver = down_cast<pm_constant_path_node_t>(call->receiver);
        if (receiver->parent != nullptr) {
            Exception::raise("Receiver is not root-anchored");
        }

        auto methodName = parser()->resolveConstant(call->name);
        if (methodName != "untyped") {
            return false;
        }

        auto receiverName = parser()->resolveConstant(receiver->name);
        return receiverName == "T";
    }

    bool Factory::isSetterCall(pm_node_t * node, const Parser &parser) {
        if (PM_NODE_TYPE(node) != PM_CALL_NODE) {
            return false;
        }

        auto *call = down_cast<pm_call_node_t>(node);
        auto methodName = parser.resolveConstant(call->name);
        return methodName.back() == '=';
    }

    bool Factory::isSafeNavigationCall(pm_node_t * node) {
        if (PM_NODE_TYPE(node) != PM_CALL_NODE) {
            return false;
        }

        return PM_NODE_FLAG_P(node, PM_CALL_NODE_FLAGS_SAFE_NAVIGATION);
    }

bool PMK::isVisibilityCall(pm_node_t *node, const Parser &parser) {
    if (PM_NODE_TYPE(node) != PM_CALL_NODE) {
        return false;
    }

    auto *call = down_cast<pm_call_node_t>(node);

    // Must have no receiver (implicit self)
    if (call->receiver != nullptr) {
        return false;
    }

    // Must have exactly one argument
    if (call->arguments == nullptr || call->arguments->arguments.size != 1) {
        return false;
    }

    // That argument must be a method definition
    pm_node_t *arg = call->arguments->arguments.nodes[0];
    if (PM_NODE_TYPE(arg) != PM_DEF_NODE) {
        return false;
    }

    // Check if the method name is a visibility modifier
    auto methodName = parser.resolveConstant(call->name);
    return methodName == "private" || methodName == "protected" || methodName == "public" ||
           methodName == "private_class_method" || methodName == "public_class_method" ||
           methodName == "package_private" || methodName == "package_private_class_method";
}

} // namespace sorbet::parser::Prism
