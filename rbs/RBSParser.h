#ifndef RBS_PARSER_H
#define RBS_PARSER_H

#include "ast/ast.h"
#include "rbs_common.h"
#include <memory>

namespace sorbet::rbs {

class RBSParser {
public:
    static VALUE parseRBS(core::MutableContext ctx, const std::string& docString, sorbet::core::LocOffsets docLoc, sorbet::core::LocOffsets methodLoc);

private:
    static VALUE parse_method_type_wrapper(VALUE string);
};

} // namespace sorbet::rbs

#endif // RBS_PARSER_H
