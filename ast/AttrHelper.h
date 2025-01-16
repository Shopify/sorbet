#ifndef SORBET_AST_ATTR_HELPER_H
#define SORBET_AST_ATTR_HELPER_H

#include "ast/ast.h"
#include "core/Context.h"

namespace sorbet::ast {
class AttrHelper {
public:
    static std::pair<core::NameRef, core::LocOffsets> getName(core::MutableContext ctx, ast::ExpressionPtr &name);
};
}; // namespace sorbet::ast

#endif
