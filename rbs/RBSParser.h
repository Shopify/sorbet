#ifndef RBS_PARSER_H
#define RBS_PARSER_H

#include "ast/ast.h"
#include "rbs_common.h"
#include <memory>

namespace sorbet::rbs {

class RBSParser {
public:
    static rbs_methodtype_t *parseSignature(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                                sorbet::core::LocOffsets methodLoc, const std::string_view docString);

    static rbs_node_t *parseType(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                               sorbet::core::LocOffsets typeLoc, const std::string_view docString);
};

} // namespace sorbet::rbs

#endif // RBS_PARSER_H
