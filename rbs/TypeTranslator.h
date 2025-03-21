#ifndef RBS_TYPE_TRANSLATOR_H
#define RBS_TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "parser/parser.h"
#include "rbs/rbs_common.h"
#include <memory>

namespace sorbet::rbs {

class TypeTranslator {
public:
    static std::unique_ptr<parser::Node>
    toParserNode(core::MutableContext ctx, const std::vector<std::pair<core::LocOffsets, core::NameRef>> &typeParams,
                 rbs_node_t *node, core::LocOffsets loc);
};

} // namespace sorbet::rbs

#endif // RBS_TYPE_TRANSLATOR_H
