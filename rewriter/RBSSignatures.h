#ifndef SORBET_REWRITER_RBS_SIGNATURES_H
#define SORBET_REWRITER_RBS_SIGNATURES_H
#include "ast/ast.h"
#include "rbs/rbs_common.h"

namespace sorbet::rewriter {

/**
 * This class rewrites RBS signatures comments into Sorbet signatures.
 *
 * So this:
 *
 *     #: (Integer) -> String
 *     def foo(x); end
 *
 * Will be rewritten to:
 *
 *     sig { params(x: Integer).returns(String) }
 *     def foo(x); end
 */
class RBSSignatures final {
private:
    // TODO: @kaan use InlinedVector<rbs::Comment, 4> for both
    struct Comments {
        /**
         * RBS annotation comments found on a method definition.
         *
         * Annotations are formatted as `@some_annotation`.
         */
        std::vector<rbs::Comment> annotations;

        /**
         * RBS signature comments found on a method definition.
         *
         * Signatures are formatted as `#: () -> void`.
         */
        std::vector<rbs::Comment> signatures;
    };
    static thread_local UnorderedMap<std::string, Comments> methodSignatures;

public:
    static void extractRBSComments(std::string_view sourceCode);
    static ast::ExpressionPtr run(core::MutableContext ctx, ast::ExpressionPtr tree);
    static Comments getMethodSignatureFor(const std::string_view &methodName, const core::LocOffsets &loc);

    RBSSignatures() = delete;
};

} // namespace sorbet::rewriter

#endif
