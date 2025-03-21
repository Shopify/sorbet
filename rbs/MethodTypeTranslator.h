#ifndef RBS_METHOD_TYPE_TRANSLATOR_H
#define RBS_METHOD_TYPE_TRANSLATOR_H

#include "ast/ast.h"
#include "parser/parser.h"
#include "rbs/rbs_common.h"

namespace sorbet::rbs {

class MethodTypeTranslator {
public:
    /**
     * Convert an RBS method signature comment to a Sorbet signature.
     *
     * For example the signature comment `#: () -> void` will be translated as `sig { void }`.
     */
    static std::unique_ptr<parser::Node> methodSignature(core::MutableContext ctx, parser::Node *def,
                                                         core::LocOffsets commentLoc, const MethodType type,
                                                         const std::vector<Comment> &annotations);

    /**
     * Convert an RBS attribute type comment to a Sorbet signature.
     *
     * For example the attribute type comment `#: Integer` will be translated as `sig { returns(Integer) }`.
     */
    static std::unique_ptr<parser::Node> attrSignature(core::MutableContext ctx, const parser::Send *send,
                                                       core::LocOffsets commentLoc, const Type type,
                                                       const std::vector<Comment> &annotations);
};

} // namespace sorbet::rbs

#endif // RBS_METHOD_TYPE_TRANSLATOR_H
