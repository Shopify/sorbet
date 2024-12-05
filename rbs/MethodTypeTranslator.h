#ifndef RBS_METHOD_TYPE_TRANSLATOR_H
#define RBS_METHOD_TYPE_TRANSLATOR_H

#include "rbs_common.h"
#include "ast/ast.h"
#include <memory>

namespace sorbet::rbs {

/**
 * A single RBS annotation comment found on a method definition.
 *
 * Annotations are formatted as `@some_annotation`.
 */
struct RBSAnnotation {
    core::LocOffsets loc;
    std::string_view string;
};

/**
 * A single RBS signature comment found on a method definition.
 *
 * Signatures are formatted as `#: () -> void`.
 */
struct RBSSignature {
    core::LocOffsets loc;
    std::string_view signature;
};

/**
 * A collection of annotations and signatures comments found on a method definition.
 */
struct MethodComments {
    std::vector<RBSAnnotation> annotations;
    std::vector<RBSSignature> signatures;
};

class MethodTypeTranslator {
public:
    /**
     * Convert an RBS method signature comment to a Sorbet signature.
     *
     * For example the signature comment `#: () -> void` will be translated as `sig { void }`.
     */
    static sorbet::ast::ExpressionPtr methodSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                      sorbet::ast::MethodDef *methodDef, rbs_methodtype_t *node,
                                                      std::vector<RBSAnnotation> annotations);

    /**
     * Convert an RBS attribute type comment to a Sorbet signature.
     */
    static sorbet::ast::ExpressionPtr attrSignature(core::MutableContext ctx, core::LocOffsets docLoc,
                                                    sorbet::ast::Send *send, rbs_node_t *node,
                                                    std::vector<RBSAnnotation> annotations);
};

} // namespace sorbet::rbs

#endif // RBS_METHOD_TYPE_TRANSLATOR_H
