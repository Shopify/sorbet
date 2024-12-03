#ifndef TYPE_TRANSLATOR_H
#define TYPE_TRANSLATOR_H

#include "rbs_common.h"
#include "ast/ast.h"
#include <memory>

namespace sorbet::rbs {

class TypeTranslator {
public:
    static sorbet::ast::ExpressionPtr toRBI(core::MutableContext ctx, rbs_node_t *node, core::LocOffsets loc);
};

} // namespace sorbet::rbs

#endif // TYPE_TRANSLATOR_H
