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

struct RBSInlineAnnotation {
    core::LocOffsets loc;
    std::string_view string;
    bool isCast; // If true, the annotation is a cast with `#:: Type` instead of `#: Type`
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
    static sorbet::ast::ExpressionPtr methodSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                      sorbet::ast::MethodDef *methodDef, VALUE methodType,
                                                      std::vector<RBSAnnotation> annotations);

    static sorbet::ast::ExpressionPtr attrSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                    sorbet::ast::Send *send, VALUE attrType,
                                                    std::vector<RBSAnnotation> annotations);
};

} // namespace sorbet::rbs

#endif // METHOD_TYPE_TRANSLATOR_H
