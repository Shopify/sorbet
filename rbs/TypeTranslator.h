#ifndef RBS_TYPE_TRANSLATOR_H
#define RBS_TYPE_TRANSLATOR_H

#include "rbs_common.h"
#include "ast/ast.h"
#include <memory>

namespace sorbet::rbs {

struct RBSInlineAnnotation {
    core::LocOffsets loc;
    std::string_view string;
    bool isCast; // If true, the annotation is a cast with `#:: Type` instead of `#: Type`
};

class TypeTranslator {
public:
    /**
     * Convert an RBS type to a Sorbet compatible expression ptr.
     *
     * For example:
     * - `Integer?` -> `T.nilable(Integer)`
     * - `(A | B)` -> `T.any(A, B)`
     */
    static sorbet::ast::ExpressionPtr toRBI(core::MutableContext ctx, rbs_node_t *node, core::LocOffsets loc);
};

} // namespace sorbet::rbs

#endif // RBS_TYPE_TRANSLATOR_H
