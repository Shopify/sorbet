#include "parser/prism/Helpers.h"
#include "parser/prism/Parser.h"
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace sorbet::parser::Prism;

namespace sorbet::parser::Prism {

namespace {
// Parser instance for node creation. Must be set via PMK::setParser()
// before using any PMK functions, and must remain alive during PMK usage.
thread_local const Parser *prismParser = nullptr;

const Parser *parser() {
    ENFORCE(prismParser != nullptr, "PMK::setParser must be called before using PMK helpers");
    return prismParser;
}

pm_arguments_node_t *createArgumentsNode(const vector<pm_node_t *> &args, core::LocOffsets loc) {
    pm_arguments_node_t *arguments = PMK::allocateNode<pm_arguments_node_t>();
    if (!arguments) {
        return nullptr;
    }

    pm_node_t **argNodes = (pm_node_t **)calloc(args.size(), sizeof(pm_node_t *));
    if (!argNodes) {
        free(arguments);
        return nullptr;
    }

    size_t argsSize = args.size();
    for (size_t i = 0; i < argsSize; i++) {
        argNodes[i] = args[i];
    }

    *arguments = (pm_arguments_node_t){.base = PMK::initializeBaseNode(PM_ARGUMENTS_NODE),
                                       .arguments = {.size = argsSize, .capacity = argsSize, .nodes = argNodes}};
    arguments->base.location = PMK::convertLocOffsets(loc);

    return arguments;
}

pm_node_t **copyNodesToArray(const vector<pm_node_t *> &nodes) {
    pm_node_t **nodeArray = (pm_node_t **)calloc(nodes.size(), sizeof(pm_node_t *));
    if (!nodeArray) {
        return nullptr;
    }

    for (size_t i = 0; i < nodes.size(); i++) {
        nodeArray[i] = nodes[i];
    }

    return nodeArray;
}

} // namespace

void PMK::setParser(const Parser *p) {
    prismParser = p;
}

pm_node_t PMK::initializeBaseNode(pm_node_type_t type) {
    pm_parser_t *prismParser = parser()->getRawParserPointer();
    prismParser->node_id++;
    uint32_t nodeId = prismParser->node_id;
    pm_location_t loc = getZeroWidthLocation();

    return (pm_node_t){.type = type, .flags = 0, .node_id = nodeId, .location = loc};
}

pm_node_t *PMK::ConstantReadNode(const char *name, core::LocOffsets loc) {
    pm_constant_id_t constantId = addConstantToPool(name);
    if (constantId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    pm_constant_read_node_t *node = allocateNode<pm_constant_read_node_t>();
    if (!node) {
        return nullptr;
    }

    *node = (pm_constant_read_node_t){.base = initializeBaseNode(PM_CONSTANT_READ_NODE), .name = constantId};

    node->base.location = convertLocOffsets(loc);

    return up_cast(node);
}

pm_node_t *PMK::ConstantWriteNode(core::LocOffsets loc, pm_constant_id_t nameId, pm_node_t *value) {
    if (!value) {
        return nullptr;
    }

    pm_constant_write_node_t *node = allocateNode<pm_constant_write_node_t>();
    if (!node) {
        return nullptr;
    }

    pm_location_t pmLoc = convertLocOffsets(loc);
    pm_location_t zeroLoc = getZeroWidthLocation();

    *node = (pm_constant_write_node_t){.base = initializeBaseNode(PM_CONSTANT_WRITE_NODE),
                                       .name = nameId,
                                       .name_loc = pmLoc,
                                       .value = value,
                                       .operator_loc = zeroLoc};
    node->base.location = pmLoc;

    return up_cast(node);
}

pm_node_t *PMK::ConstantPathNode(core::LocOffsets loc, pm_node_t *parent, const char *name) {
    pm_constant_id_t nameId = addConstantToPool(name);
    if (nameId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    pm_constant_path_node_t *node = allocateNode<pm_constant_path_node_t>();
    if (!node) {
        return nullptr;
    }

    pm_location_t pmLoc = convertLocOffsets(loc);

    *node = (pm_constant_path_node_t){.base = initializeBaseNode(PM_CONSTANT_PATH_NODE),
                                      .parent = parent,
                                      .name = nameId,
                                      .delimiter_loc = pmLoc,
                                      .name_loc = pmLoc};

    // The translator uses node->location (i.e., node->base.location) to assign locations to
    // the translated Whitequark AST nodes. Without this, nodes get {0,0} locations, breaking
    // features like check-and-and creation.
    node->base.location = pmLoc;

    return up_cast(node);
}

pm_node_t *PMK::SingleArgumentNode(pm_node_t *arg) {
    vector<pm_node_t *> args = {arg};
    pm_arguments_node_t *arguments = createArgumentsNode(args, core::LocOffsets::none());
    if (!arguments) {
        return nullptr;
    }

    arguments->base.location = arg->location;
    return up_cast(arguments);
}

pm_node_t *PMK::Self(core::LocOffsets loc) {
    pm_self_node_t *selfNode = allocateNode<pm_self_node_t>();
    if (!selfNode) {
        return nullptr;
    }

    *selfNode = (pm_self_node_t){.base = initializeBaseNode(PM_SELF_NODE)};
    if (loc != core::LocOffsets::none()) {
        selfNode->base.location = convertLocOffsets(loc);
    }

    return up_cast(selfNode);
}

pm_constant_id_t PMK::addConstantToPool(const char *name) {
    pm_parser_t *prismParser = parser()->getRawParserPointer();
    size_t nameLen = strlen(name);
    uint8_t *stable = (uint8_t *)calloc(nameLen, sizeof(uint8_t));
    if (!stable) {
        return PM_CONSTANT_ID_UNSET;
    }
    memcpy(stable, name, nameLen);
    pm_constant_id_t id = pm_constant_pool_insert_owned(&prismParser->constant_pool, stable, nameLen);
    return id;
}

pm_location_t PMK::getZeroWidthLocation() {
    pm_parser_t *prismParser = parser()->getRawParserPointer();
    const uint8_t *sourceStart = prismParser->start;
    return {.start = sourceStart, .end = sourceStart};
}

pm_location_t PMK::convertLocOffsets(core::LocOffsets loc) {
    pm_parser_t *prismParser = parser()->getRawParserPointer();
    const uint8_t *sourceStart = prismParser->start;

    const uint8_t *startPtr = sourceStart + loc.beginPos();
    const uint8_t *endPtr = sourceStart + loc.endPos();

    return {.start = startPtr, .end = endPtr};
}

pm_call_node_t *PMK::createSendNode(pm_node_t *receiver, pm_constant_id_t methodId, pm_node_t *arguments,
                                    pm_location_t messageLoc, pm_location_t fullLoc, pm_location_t tinyLoc,
                                    pm_node_t *block) {
    pm_call_node_t *call = allocateNode<pm_call_node_t>();
    if (!call) {
        return nullptr;
    }

    *call = (pm_call_node_t){.base = initializeBaseNode(PM_CALL_NODE),
                             .receiver = receiver,
                             .call_operator_loc = tinyLoc,
                             .name = methodId,
                             .message_loc = messageLoc,
                             .opening_loc = tinyLoc,
                             .arguments = down_cast<pm_arguments_node_t>(arguments),
                             .closing_loc = tinyLoc,
                             .block = block};
    call->base.location = fullLoc;

    return call;
}

pm_node_t *PMK::SymbolFromConstant(core::LocOffsets nameLoc, pm_constant_id_t nameId) {
    if (nameId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    auto nameView = parser()->resolveConstant(nameId);
    size_t nameSize = nameView.size();

    uint8_t *stable = (uint8_t *)calloc(nameSize, sizeof(uint8_t));
    if (!stable) {
        return nullptr;
    }
    memcpy(stable, nameView.data(), nameSize);

    pm_symbol_node_t *symbolNode = allocateNode<pm_symbol_node_t>();
    if (!symbolNode) {
        free(stable);
        return nullptr;
    }

    pm_location_t location = convertLocOffsets(nameLoc.copyWithZeroLength());

    pm_string_t unescapedString;
    unescapedString.source = stable;
    unescapedString.length = nameSize;

    *symbolNode = (pm_symbol_node_t){.base = initializeBaseNode(PM_SYMBOL_NODE),
                                     .opening_loc = location,
                                     .value_loc = location,
                                     .closing_loc = location,
                                     .unescaped = unescapedString};
    symbolNode->base.location = location;

    return up_cast(symbolNode);
}

pm_node_t *PMK::AssocNode(core::LocOffsets loc, pm_node_t *key, pm_node_t *value) {
    if (!key || !value) {
        return nullptr;
    }

    pm_assoc_node_t *assocNode = allocateNode<pm_assoc_node_t>();
    if (!assocNode) {
        return nullptr;
    }

    pm_location_t location = convertLocOffsets(loc.copyWithZeroLength());

    *assocNode = (pm_assoc_node_t){
        .base = initializeBaseNode(PM_ASSOC_NODE), .key = key, .value = value, .operator_loc = location};
    assocNode->base.location = location;

    return up_cast(assocNode);
}

pm_node_t *PMK::Hash(core::LocOffsets loc, const vector<pm_node_t *> &pairs) {
    if (pairs.empty()) {
        return nullptr;
    }

    pm_hash_node_t *hashNode = allocateNode<pm_hash_node_t>();
    if (!hashNode) {
        return nullptr;
    }

    pm_node_t **elements = copyNodesToArray(pairs);
    if (!elements) {
        free(hashNode);
        return nullptr;
    }

    pm_location_t baseLoc = convertLocOffsets(loc);
    pm_location_t openingLoc = {.start = nullptr, .end = nullptr};
    pm_location_t closingLoc = {.start = nullptr, .end = nullptr};

    size_t pairsSize = pairs.size();
    *hashNode = (pm_hash_node_t){.base = initializeBaseNode(PM_HASH_NODE),
                                 .opening_loc = openingLoc,
                                 .elements = {.size = pairsSize, .capacity = pairsSize, .nodes = elements},
                                 .closing_loc = closingLoc};
    hashNode->base.location = baseLoc;

    return up_cast(hashNode);
}

pm_node_t *PMK::KeywordHash(core::LocOffsets loc, const vector<pm_node_t *> &pairs) {
    if (pairs.empty()) {
        return nullptr;
    }

    pm_keyword_hash_node_t *hashNode = allocateNode<pm_keyword_hash_node_t>();
    if (!hashNode) {
        return nullptr;
    }

    pm_node_t **elements = copyNodesToArray(pairs);
    if (!elements) {
        free(hashNode);
        return nullptr;
    }

    pm_location_t baseLoc = convertLocOffsets(loc);

    size_t pairsSize = pairs.size();
    *hashNode = (pm_keyword_hash_node_t){.base = initializeBaseNode(PM_KEYWORD_HASH_NODE),
                                         .elements = {.size = pairsSize, .capacity = pairsSize, .nodes = elements}};
    hashNode->base.location = baseLoc;

    return up_cast(hashNode);
}

pm_node_t *PMK::SorbetPrivateStatic(core::LocOffsets loc) {
    // Build a root-anchored constant path ::Sorbet::Private::Static
    pm_node_t *sorbet = ConstantPathNode(loc, nullptr, "Sorbet");
    if (!sorbet) {
        return nullptr;
    }

    pm_node_t *sorbetPrivate = ConstantPathNode(loc, sorbet, "Private");
    if (!sorbetPrivate) {
        return nullptr;
    }

    return ConstantPathNode(loc, sorbetPrivate, "Static");
}

pm_node_t *PMK::TSigWithoutRuntime(core::LocOffsets loc) {
    // Build a root-anchored constant path ::T::Sig::WithoutRuntime
    pm_node_t *tConst = ConstantPathNode(loc, nullptr, "T");
    if (!tConst) {
        return nullptr;
    }

    pm_node_t *tSig = ConstantPathNode(loc, tConst, "Sig");
    if (!tSig) {
        return nullptr;
    }

    return ConstantPathNode(core::LocOffsets::none(), tSig, "WithoutRuntime");
}

pm_node_t *PMK::Symbol(core::LocOffsets nameLoc, const char *name) {
    if (!name) {
        return nullptr;
    }

    pm_constant_id_t nameId = addConstantToPool(name);
    return SymbolFromConstant(nameLoc, nameId);
}

pm_node_t *PMK::Send0(core::LocOffsets loc, pm_node_t *receiver, const char *method) {
    if (!receiver || !method) {
        return nullptr;
    }

    pm_constant_id_t methodId = addConstantToPool(method);
    if (methodId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    pm_location_t fullLoc = convertLocOffsets(loc);
    pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

    return up_cast(createSendNode(receiver, methodId, nullptr, tinyLoc, fullLoc, tinyLoc));
}

pm_node_t *PMK::Send1(core::LocOffsets loc, pm_node_t *receiver, const char *method, pm_node_t *arg1) {
    if (!receiver || !method || !arg1) {
        return nullptr;
    }

    pm_constant_id_t methodId = addConstantToPool(method);
    if (methodId == PM_CONSTANT_ID_UNSET) {
        return nullptr;
    }

    pm_node_t *arguments = SingleArgumentNode(arg1);
    if (!arguments) {
        return nullptr;
    }

    pm_location_t fullLoc = convertLocOffsets(loc);
    pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

    return up_cast(createSendNode(receiver, methodId, arguments, tinyLoc, fullLoc, tinyLoc));
}

pm_node_t *PMK::Send(core::LocOffsets loc, pm_node_t *receiver, const char *method, const vector<pm_node_t *> &args,
                     pm_node_t *block) {
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

    return up_cast(
        createSendNode(receiver, methodId, arguments ? up_cast(arguments) : nullptr, tinyLoc, fullLoc, tinyLoc, block));
}

pm_node_t *PMK::T(core::LocOffsets loc) {
    // Create ::T constant path node
    return ConstantPathNode(loc, nullptr, "T");
}

pm_node_t *PMK::TUntyped(core::LocOffsets loc) {
    // Create T.untyped call
    pm_node_t *tConst = T(loc);
    if (!tConst) {
        return nullptr;
    }

    return Send0(loc, tConst, "untyped");
}

pm_node_t *PMK::TNilable(core::LocOffsets loc, pm_node_t *type) {
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

    pm_location_t fullLoc = convertLocOffsets(loc);
    pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

    return up_cast(createSendNode(tConst, methodId, up_cast(arguments), tinyLoc, fullLoc, tinyLoc));
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

    pm_location_t fullLoc = convertLocOffsets(loc);
    pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

    return up_cast(createSendNode(tConst, methodId, up_cast(arguments), tinyLoc, fullLoc, tinyLoc));
}

pm_node_t *PMK::TTypeParameter(core::LocOffsets loc, pm_node_t *name) {
    // Create T.type_parameter(name) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !name) {
        return nullptr;
    }

    return Send1(loc, tConst, "type_parameter", name);
}

pm_node_t *PMK::TProc(core::LocOffsets loc, pm_node_t *args, pm_node_t *returnType) {
    // Create T.proc.params(args).returns(returnType) call
    pm_node_t *builder = T(loc);
    if (!builder || !returnType) {
        return nullptr;
    }

    builder = Send0(loc, builder, "proc");
    if (!builder) {
        return nullptr;
    }

    if (args != nullptr) {
        builder = Send1(loc, builder, "params", args);
        if (!builder) {
            return nullptr;
        }
    }

    return Send1(loc, builder, "returns", returnType);
}

pm_node_t *PMK::TProcVoid(core::LocOffsets loc, pm_node_t *args) {
    // Create T.proc.params(args).void call
    pm_node_t *builder = T(loc);
    if (!builder) {
        return nullptr;
    }

    builder = Send0(loc, builder, "proc");
    if (!builder) {
        return nullptr;
    }

    if (args != nullptr) {
        builder = Send1(loc, builder, "params", args);
        if (!builder) {
            return nullptr;
        }
    }

    return Send0(loc, builder, "void");
}

pm_node_t *PMK::Send2(core::LocOffsets loc, pm_node_t *receiver, const char *method, pm_node_t *arg1, pm_node_t *arg2) {
    if (!receiver || !method || !arg1 || !arg2) {
        return nullptr;
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

    pm_location_t fullLoc = convertLocOffsets(loc);
    pm_location_t tinyLoc = convertLocOffsets(loc.copyWithZeroLength());

    return up_cast(createSendNode(receiver, methodId, up_cast(arguments), tinyLoc, fullLoc, tinyLoc));
}

pm_node_t *PMK::TLet(core::LocOffsets loc, pm_node_t *value, pm_node_t *type) {
    // Create T.let(value, type) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !value || !type) {
        return nullptr;
    }

    return Send2(loc, tConst, "let", value, type);
}

pm_node_t *PMK::TCast(core::LocOffsets loc, pm_node_t *value, pm_node_t *type) {
    // Create T.cast(value, type) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !value || !type) {
        return nullptr;
    }

    return Send2(loc, tConst, "cast", value, type);
}

pm_node_t *PMK::TMust(core::LocOffsets loc, pm_node_t *value) {
    // Create T.must(value) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !value) {
        return nullptr;
    }

    return Send1(loc, tConst, "must", value);
}

pm_node_t *PMK::TUnsafe(core::LocOffsets loc, pm_node_t *value) {
    // Create T.unsafe(value) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !value) {
        return nullptr;
    }

    return Send1(loc, tConst, "unsafe", value);
}

pm_node_t *PMK::TAbsurd(core::LocOffsets loc, pm_node_t *value) {
    // Create T.absurd(value) call
    pm_node_t *tConst = T(loc);
    if (!tConst || !value) {
        return nullptr;
    }

    return Send1(loc, tConst, "absurd", value);
}

pm_node_t *PMK::TBindSelf(core::LocOffsets loc, pm_node_t *type) {
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

pm_node_t *PMK::TTypeAlias(core::LocOffsets loc, pm_node_t *type) {
    pm_node_t *tConst = T(loc);
    if (!tConst || !type) {
        return nullptr;
    }

    pm_node_t *send = Send0(loc, tConst, "type_alias");
    if (!send) {
        return nullptr;
    }

    // Create statements node containing the type
    pm_statements_node_t *stmts = allocateNode<pm_statements_node_t>();
    if (!stmts) {
        return nullptr;
    }
    *stmts = (pm_statements_node_t){.base = initializeBaseNode(PM_STATEMENTS_NODE),
                                    .body = {.size = 0, .capacity = 0, .nodes = nullptr}};
    stmts->base.location = convertLocOffsets(loc);
    pm_node_list_append(&stmts->body, type);

    // Create block node with the send and statements
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
    block->base.location = convertLocOffsets(loc);

    // Set the block on the call node
    auto *call = down_cast<pm_call_node_t>(send);
    call->block = up_cast(block);

    return send;
}

pm_node_t *PMK::Array(core::LocOffsets loc, const vector<pm_node_t *> &elements) {
    // Create an array node
    pm_array_node_t *array = allocateNode<pm_array_node_t>();
    if (!array) {
        return nullptr;
    }

    pm_node_t **elemNodes = nullptr;
    size_t elementsSize = elements.size();
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

pm_node_t *PMK::T_Array(core::LocOffsets loc) {
    // Create T::Array constant path
    return ConstantPathNode(loc, T(loc), "Array");
}

pm_node_t *PMK::T_Class(core::LocOffsets loc) {
    // Create T::Class constant path
    return ConstantPathNode(loc, T(loc), "Class");
}

pm_node_t *PMK::T_Enumerable(core::LocOffsets loc) {
    // Create T::Enumerable constant path
    return ConstantPathNode(loc, T(loc), "Enumerable");
}

pm_node_t *PMK::T_Enumerator(core::LocOffsets loc) {
    // Create T::Enumerator constant path
    return ConstantPathNode(loc, T(loc), "Enumerator");
}

pm_node_t *PMK::T_Hash(core::LocOffsets loc) {
    // Create T::Hash constant path
    return ConstantPathNode(loc, T(loc), "Hash");
}

pm_node_t *PMK::T_Set(core::LocOffsets loc) {
    // Create T::Set constant path
    return ConstantPathNode(loc, T(loc), "Set");
}

pm_node_t *PMK::T_Range(core::LocOffsets loc) {
    // Create T::Range constant path
    return ConstantPathNode(loc, T(loc), "Range");
}

bool PMK::isTUntyped(pm_node_t *node) {
    if (!node || PM_NODE_TYPE(node) != PM_CALL_NODE) {
        return false;
    }

    pm_call_node_t *call = down_cast<pm_call_node_t>(node);
    if (!call->receiver || PM_NODE_TYPE(call->receiver) != PM_CONSTANT_PATH_NODE) {
        return false;
    }

    // Check if receiver is ::T and method is "untyped"
    pm_constant_path_node_t *receiver = down_cast<pm_constant_path_node_t>(call->receiver);
    if (receiver->parent != nullptr) {
        return false; // Should be root-anchored ::T
    }

    auto methodName = parser()->resolveConstant(call->name);
    if (methodName != "untyped") {
        return false;
    }

    auto receiverName = parser()->resolveConstant(receiver->name);
    return receiverName == "T";
}

bool PMK::isSetterCall(pm_node_t *node, const Parser &parser) {
    if (PM_NODE_TYPE(node) != PM_CALL_NODE) {
        return false;
    }

    auto *call = down_cast<pm_call_node_t>(node);
    auto methodName = parser.resolveConstant(call->name);
    return !methodName.empty() && methodName.back() == '=';
}

bool PMK::isSafeNavigationCall(pm_node_t *node) {
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
