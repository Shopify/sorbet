#ifndef SORBET_PARSER_PARSE_RESULT_H
#define SORBET_PARSER_PARSE_RESULT_H

#include "Node.h"
#include "core/LocOffsets.h"

namespace sorbet::parser {

struct ParseResult {
    std::unique_ptr<Node> tree;
    std::vector<core::LocOffsets> commentLocations;
};

} // namespace sorbet::parser

#endif // SORBET_PARSER_PARSE_RESULT_H
