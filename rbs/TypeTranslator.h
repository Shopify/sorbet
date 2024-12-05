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

    /**
     * Get the location offset of an RBS node.
     */
    static core::LocOffsets nodeLoc(core::MutableContext ctx, core::LocOffsets offset, rbs_node_t *node);

    /**
     * Get the location offset from a RBS location.
     */
    static core::LocOffsets locOffsets(core::LocOffsets offset, rbs_location_t *docLoc);
};

} // namespace sorbet::rbs

#endif // RBS_TYPE_TRANSLATOR_H
