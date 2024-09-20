#ifndef RBS_PARSER_H
#define RBS_PARSER_H

#include "ast/ast.h"
#include "rbs_common.h"
#include <memory>

namespace sorbet::rbs {

class RBSParser {
public:
    static VALUE parseSignature(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                                sorbet::core::LocOffsets methodLoc, const std::string_view docString);

    static VALUE parseType(core::MutableContext ctx, sorbet::core::LocOffsets docLoc, sorbet::core::LocOffsets typeLoc,
                           const std::string_view docString);

private:
    static VALUE parse_method_type_wrapper(VALUE string);
    static VALUE parse_type_wrapper(VALUE string);
};

} // namespace sorbet::rbs

#endif // RBS_PARSER_H
