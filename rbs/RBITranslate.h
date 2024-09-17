#ifndef METHOD_TYPE_VISITOR_H
#define METHOD_TYPE_VISITOR_H

#include "ast/ast.h"
#include "ruby.h"
#include <memory>

namespace sorbet::rbs {

/**
 * TODO
 */
class RBITranslate {
public:
    sorbet::ast::ExpressionPtr type_to_rbi(VALUE type);

private:
    core::MutableContext ctx;
    sorbet::ast::MethodDef *methodDef;
    sorbet::ast::ExpressionPtr createSig();
    void visitParameterList(VALUE parameterList);
    void visitType(VALUE type);
    sorbet::ast::ExpressionPtr translateType(VALUE type);

    sorbet::ast::ExpressionPtr translateClassInstanceType(core::LocOffsets loc, VALUE type);
    // Add more private methods as needed
};

} // namespace sorbet::rbs

#endif // METHOD_TYPE_VISITOR_H
