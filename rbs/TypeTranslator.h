#ifndef TYPE_TRANSLATOR_H
#define TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "ruby.h"
#include <memory>

namespace sorbet::rbs {

class TypeTranslator {
public:
    static sorbet::ast::ExpressionPtr toRBI(core::MutableContext ctx, VALUE type, core::LocOffsets loc);
};

} // namespace sorbet::rbs

#endif // TYPE_TRANSLATOR_H
