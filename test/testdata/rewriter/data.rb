# typed: true
require_relative "../../t"

module Foo
    class Data
    end
end

class NotData
    B = T.let(Foo::Data.new, Foo::Data)
    var = Data.define(:foo)
end

class NullData
    N = Data.define
end

class RealData
    A = Data.define(:foo, :bar)
end

class RealDataDesugar
    class A < Data
        extend T::Sig
        def foo; end
        def bar; end
        sig {params(foo: BasicObject, bar: BasicObject).returns(A)}
        def self.new(foo=nil, bar=nil)
            T.cast(nil, A)
        end
    end
end

class TwoDatas
    A = Data.define(:foo)
    B = Data.define(:foo)
end

class AccidentallyData
    class Data
      def self.define; end
    end

    # We do this in the Rewriter pass before we've typeAlias the constants
    A = Data.define(:foo, :bar)
end

class InvalidMember
  A = Data.define(:foo=) # error: Data member `foo=` cannot end with an equal
end

class MixinData
  module MyMixin
    def foo; end
  end

  MyData = Data.define(:x) do
    include MyMixin
    self.new(1).x
    self.new(1).foo
  end

  MyData.new(1).x
  MyData.new(1).foo
end

class BadUsages
  A = Data.define(giberish: 1)
  #               ^^^^^^^^^^^ error: Expected `T.any(Symbol, String)` but found `{giberish: Integer(1)}` for argument `args`

  B = Data.define(:b)
  b_data = B.new(1)
  b_data.b = 6 # error: Setter method `b=` does not exist on `BadUsages::B`
end

class Main
    def main
        a = Data.define(:foo)
        # a.is_a?(Data) is actually false, because `Data.define` dynamically
        # allocates and returns a class object for this struct, but we don't
        # have a great way to model that statically in the case where the
        # result is assigned to a local variable, not a constant.
        T.assert_type!(a, Data)
        T.assert_type!(a.new(2), Data)

        # This should raise a "Not enough arguments" error, but it doesn't because the rewriter
        # currently doesn't know how to typecheck when LHS is an ident instead of a constant.
        # Is this okay?
        a.new

        T.assert_type!(RealData::A.new(2, 3), RealData::A)

        T.assert_type!(RealDataDesugar::A.new(2, 3), RealDataDesugar::A)
    end
end
puts Main.new.main

class FullyQualifiedDataUsages
  Foo = Data.define(:a)
  Bar = ::Data.define(:a)
  Baz = ::Foo::Data.new
  Quux = Data.define

  Foo.new(1).a
  Bar.new(1).a
  Quux.new()
end

# ============================================================================
# Typed Data.define with Sorbet sig { } blocks
# ============================================================================

# Basic typed members with sig + def initialize(...) = super
TypedWithSorbetSig = Data.define(:amount, :currency) do
  extend T::Sig

  sig { params(amount: Numeric, currency: String).void }
  def initialize(amount:, currency:) = super
end
T.reveal_type(TypedWithSorbetSig.new(amount: 10, currency: "USD").amount) # error: Revealed type: `Numeric`
T.reveal_type(TypedWithSorbetSig.new(amount: 10, currency: "USD").currency) # error: Revealed type: `String`
TypedWithSorbetSig.new(amount: "bad", currency: "USD") # error: Expected `Numeric` but found `String("bad")` for argument `amount`

# Complex types with Sorbet sig
SorbetSigComplex = Data.define(:items, :label) do
  extend T::Sig

  sig { params(items: T::Array[Integer], label: T.nilable(String)).void }
  def initialize(items:, label:) = super
end
T.reveal_type(SorbetSigComplex.new(items: [1], label: nil).items) # error: Revealed type: `T::Array[Integer]`
T.reveal_type(SorbetSigComplex.new(items: [1], label: nil).label) # error: Revealed type: `T.nilable(String)`

# Sorbet sig with additional methods
SorbetSigWithMethods = Data.define(:name) do
  extend T::Sig

  sig { params(name: String).void }
  def initialize(name:) = super

  sig { returns(String) }
  def greeting
    "Hello, #{name}!"
  end
end
T.reveal_type(SorbetSigWithMethods.new(name: "alice").name) # error: Revealed type: `String`

# Sorbet sig: initialize without bare super — readers stay untyped.
# When the initialize body transforms values (not bare `super`), we can't
# reliably propagate sig types to readers, so Sorbet conservatively falls
# back to untyped.
SorbetSigNoBareSuper = Data.define(:x) do
  extend T::Sig

  sig { params(x: Integer).void }
  def initialize(x:)
    super(x: x * 2)
  end
end
T.reveal_type(SorbetSigNoBareSuper.new(x: 1).x) # error: Revealed type: `T.untyped`
