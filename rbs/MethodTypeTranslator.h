#ifndef METHOD_TYPE_TRANSLATOR_H
#define METHOD_TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "ruby.h"
#include <memory>

namespace sorbet::rbs {

/**
 * TODO
 */
class MethodTypeTranslator {
public:
    MethodTypeTranslator(core::MutableContext ctx, sorbet::ast::MethodDef *methodDef, VALUE methodType);
    sorbet::ast::ExpressionPtr to_rbi();

private:
    core::MutableContext ctx;
    sorbet::ast::MethodDef *methodDef;
    VALUE methodType;

    sorbet::ast::ExpressionPtr translateType(VALUE type);
    sorbet::ast::ExpressionPtr translateClassInstanceType(core::LocOffsets loc, VALUE type);
};

} // namespace sorbet::rbs

#endif // METHOD_TYPE_TRANSLATOR_H
