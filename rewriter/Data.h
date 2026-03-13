#ifndef SORBET_REWRITER_DATA_H
#define SORBET_REWRITER_DATA_H
#include "ast/ast.h"

namespace sorbet::rewriter {

/**
 * This class desugars things of the form
 *
 *   A = Data.define(:foo, :bar)
 *
 * into
 *
 *   class A < Data
 *       sig {returns(T.untyped)}
 *       def foo; end
 *       sig {returns(T.untyped)}
 *       def bar; end
 *       sig {params(foo: T.untyped, bar: T.untyped).returns(T.attached_class)}
 *       def self.new(foo=nil, bar=nil); end
 *   end
 *
 * When an initialize method with a sig is provided and calls bare super:
 *
 *   A = Data.define(:foo, :bar) do
 *     extend T::Sig
 *     sig { params(foo: Integer, bar: String).void }
 *     def initialize(foo:, bar:) = super
 *   end
 *
 * the typed members are propagated to the attribute readers:
 *
 *   class A < Data
 *       sig {returns(Integer)}
 *       def foo; end
 *       sig {returns(String)}
 *       def bar; end
 *       sig {params(foo: Integer, bar: String).void}
 *       def initialize(foo:, bar:); end
 *   end
 *
 * Design notes:
 *
 * - `self.[]` is intentionally left untyped even when a typed initialize is
 *   provided. Sorbet cannot overload a method to accept both positional and
 *   keyword arguments, so typing only the keyword-based `new`/`initialize`
 *   path is the conservative choice — it avoids falsely constraining valid
 *   usage of `self.[]` while still providing full type safety for the
 *   recommended `new(foo:, bar:)` form.
 *
 * - A non-void return type on the initialize sig (e.g. `returns(Integer)`)
 *   is not rejected by this rewriter. This is consistent with how Sorbet
 *   handles initialize sigs elsewhere — the return type is effectively
 *   ignored since initialize always returns void at the language level.
 *   We do not add special validation here to keep the rewriter focused on
 *   propagating member types rather than enforcing sig correctness, which
 *   is handled by later phases.
 */
class Data final {
public:
    static std::vector<ast::ExpressionPtr> run(core::MutableContext ctx, ast::Assign *asgn);

    Data() = delete;
};

} // namespace sorbet::rewriter

#endif
