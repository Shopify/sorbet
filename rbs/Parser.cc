#include "rbs/Parser.h"

namespace sorbet::rbs {

namespace {

// RBS's parser needs to initialize a global constant pool to host around 26 unique strings
// that are shared across all parsers.
// When the process exits, we need to free the constant pool too.
// This class and the static rbsLibraryInitializer variable leverage C++'s static initialization
// and destruction semantics to automatically initialize the constant pool at program startup
// and clean it up at program termination.
class RBSLibraryInitializer {
public:
    RBSLibraryInitializer() {
        const size_t num_uniquely_interned_strings = 26;
        rbs_constant_pool_init(RBS_GLOBAL_CONSTANT_POOL, num_uniquely_interned_strings);
    }

    ~RBSLibraryInitializer() {
        rbs_constant_pool_free(RBS_GLOBAL_CONSTANT_POOL);
    }
};

RBSLibraryInitializer rbsLibraryInitializer;
} // namespace

Parser::Parser(rbs_string_t rbsString, const rbs_encoding_t *encoding)
    : parser(rbs_parser_new(rbsString, encoding, 0, rbsString.end - rbsString.start), rbs_parser_free) {}

std::string_view Parser::resolveConstant(const rbs_ast_symbol_t *symbol) const {
    auto constant = rbs_constant_pool_id_to_constant(&parser->constant_pool, symbol->constant_id);
    return std::string_view(reinterpret_cast<const char *>(constant->start), constant->length);
}

rbs_method_type_t *Parser::parseMethodType() {
    rbs_method_type_t *methodType = nullptr;
    rbs_parse_method_type(parser.get(), &methodType);
    return methodType;
}

rbs_node_t *Parser::parseType() {
    rbs_node_t *type = nullptr;
    rbs_parse_type(parser.get(), &type);
    return type;
}

rbs_ast_members_method_definition_t *Parser::parseMemberDefinition() {
    bool instance_only = false;
    bool accept_overload = false;
    rbs_position_t comment_pos;
    rbs_node_list_t *annotations = nullptr;
    rbs_ast_members_method_definition_t *methodDefinition = nullptr;
    rbs_parse_member_def(parser.get(), instance_only, accept_overload, comment_pos, annotations, &methodDefinition);
    return methodDefinition;
}

bool Parser::hasError() const {
    return parser->error != nullptr;
}

const rbs_error_t *Parser::getError() const {
    return parser->error;
}

} // namespace sorbet::rbs
