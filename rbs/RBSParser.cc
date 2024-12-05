#include "RBSParser.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/GlobalState.h"
#include "core/Names.h"
#include "core/errors/rewriter.h"
#include <cstring>
#include <functional>

namespace sorbet::rbs {

rbs_methodtype_t *RBSParser::parseSignature(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                                     sorbet::core::LocOffsets methodLoc, const std::string_view docString) {

    rbs_string_t rbsString = {
        .start = docString.data(),
        .end = docString.data() + docString.size(),
        .type = rbs_string_t::RBS_STRING_SHARED,
    };

    const rbs_encoding_t *encoding = &rbs_encodings[RBS_ENCODING_UTF_8];

    lexstate *lexer = alloc_lexer(rbsString, encoding, 0, docString.size());
    parserstate *parser = alloc_parser(lexer, 0, docString.size());

    rbs_methodtype_t *rbsMethodType = nullptr;
    parse_method_type(parser, &rbsMethodType);

    if (parser->error) {
        range range = parser->error->token.range;
        auto startColumn = range.start.char_pos + 2;
        auto endColumn = range.end.char_pos + 2;

        core::LocOffsets offset{docLoc.beginPos() + startColumn, docLoc.beginPos() + endColumn};
        if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
            e.setHeader("Failed to parse RBS signature ({})", parser->error->message);
        }

        return nullptr;
    }

    return rbsMethodType;
}

rbs_node_t *RBSParser::parseType(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                                sorbet::core::LocOffsets typeLoc, const std::string_view docString) {

    rbs_string_t rbsString = {
        .start = docString.data(),
        .end = docString.data() + docString.size(),
        .type = rbs_string_t::RBS_STRING_SHARED,
    };

    const rbs_encoding_t *encoding = &rbs_encodings[RBS_ENCODING_UTF_8];

    lexstate *lexer = alloc_lexer(rbsString, encoding, 0, docString.size());
    parserstate *parser = alloc_parser(lexer, 0, docString.size());

    rbs_node_t *rbsType = nullptr;
    parse_type(parser, &rbsType);

    if (parser->error) {
        range range = parser->error->token.range;
        auto startColumn = range.start.char_pos + 2;
        auto endColumn = range.end.char_pos + 2;

        core::LocOffsets offset{docLoc.beginPos() + startColumn, docLoc.beginPos() + endColumn};
        if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
            e.setHeader("Failed to parse RBS signature ({})", parser->error->message);
        }

        return nullptr;
    }

    return rbsType;
}

} // namespace sorbet::rbs
