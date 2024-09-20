#ifndef METHOD_TYPE_TRANSLATOR_H
#define METHOD_TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "ruby.h"
#include <memory>

namespace sorbet::rbs {

struct RBSAnnotation {
    core::LocOffsets loc;
    std::string_view string;
};

struct RBSSignature {
    core::LocOffsets loc;
    std::string_view signature;
};

struct MethodComments {
    std::vector<RBSAnnotation> annotations;
    std::vector<RBSSignature> signatures;
};

class MethodTypeTranslator {
public:
    static sorbet::ast::ExpressionPtr toRBI(core::MutableContext ctx, core::LocOffsets docLoc,
                                            sorbet::ast::MethodDef *methodDef, VALUE methodType,
                                            std::vector<RBSAnnotation> annotations);
};

} // namespace sorbet::rbs

#endif // METHOD_TYPE_TRANSLATOR_H
