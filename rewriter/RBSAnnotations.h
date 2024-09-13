#ifndef SORBET_REWRITER_RBS_ANNOTATIONS_H
#define SORBET_REWRITER_RBS_ANNOTATIONS_H
#include "ast/ast.h"

namespace sorbet::rewriter {

/**
 * TODO
 */
class RBSAnnotations final {
public:
    static ast::ExpressionPtr run(core::Context ctx, ast::ExpressionPtr tree);

    RBSAnnotations() = delete;
};

} // namespace sorbet::rewriter

#endif
