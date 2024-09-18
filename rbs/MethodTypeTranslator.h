#ifndef METHOD_TYPE_TRANSLATOR_H
#define METHOD_TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "ruby.h"
#include <memory>

namespace sorbet::rbs {

class MethodTypeTranslator {
public:
    static sorbet::ast::ExpressionPtr toRBI(core::MutableContext ctx, core::LocOffsets docLoc,
                                            sorbet::ast::MethodDef *methodDef, VALUE methodType);
};

} // namespace sorbet::rbs

#endif // METHOD_TYPE_TRANSLATOR_H
