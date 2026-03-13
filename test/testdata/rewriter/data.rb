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
# Custom initialize without sig (untyped, but initialize args are validated)
# ============================================================================

SquaredPoint = Data.define(:x, :y) do
  def initialize(x:, y:)
    super(x: x ** 2, y: y ** 2)
  end
end

BadSquaredPoint = Data.define(:x, :y) do
  def initialize(x:) # error: Method `BadSquaredPoint#initialize` redefined without matching argument count. Expected: `2`, got: `1`
    super(x: x ** 2, y: 10)
  end
end

# ============================================================================
# Typed Data.define — the core new feature
# ============================================================================

module TypedData
  # Basic typed members with sig + `def initialize(...) = super`
  BasicMoney = Data.define(:amount, :currency) do
    extend T::Sig

    sig { params(amount: Numeric, currency: String).void }
    def initialize(amount:, currency:) = super
  end

  # Type coercion in initialize body — falls back to untyped (not bare super)
  MoneyWithTypeCoercionIsUntyped = Data.define(:amount, :currency) do
    extend T::Sig

    sig { params(amount: T.any(Numeric, String, Time), currency: String).void }
    def initialize(amount:, currency:) = super(amount: amount.to_i, currency:)
  end
end

# ============================================================================
# Migration patterns: T::Struct const-only → Data.define
# ============================================================================

module MigrationPatterns
  # Simple Data.define with no block — all members are T.untyped
  SimpleData = Data.define(:bar, :baz)

  # Data.define with typed initialize (migration target for typed T::Struct)
  TypedStruct = Data.define(:bar, :baz) do
    extend T::Sig

    sig { params(bar: String, baz: Integer).void }
    def initialize(bar:, baz:) = super
  end

  # Data.define with custom methods (common migration pattern)
  DataWithMethods = Data.define(:user) do
    extend T::Sig

    sig { params(user: String).void }
    def initialize(user:) = super

    sig { returns(T::Boolean) }
    def qualifies?
      !user.empty?
    end
  end

  # Data.define with include (predicate pattern)
  module PredicateInterface; end

  DataWithInclude = Data.define(:user) do
    include PredicateInterface
    extend T::Sig

    sig { params(user: String).void }
    def initialize(user:) = super
  end
end

# ============================================================================
# Complex type annotations
# ============================================================================

module ComplexTypes
  # T.nilable types
  NilableData = Data.define(:name, :email) do
    extend T::Sig

    sig { params(name: String, email: T.nilable(String)).void }
    def initialize(name:, email:) = super
  end

  # T.any union types
  UnionData = Data.define(:value) do
    extend T::Sig

    sig { params(value: T.any(String, Integer, Symbol)).void }
    def initialize(value:) = super
  end

  # Generic collection types
  CollectionData = Data.define(:items, :metadata) do
    extend T::Sig

    sig { params(items: T::Array[String], metadata: T::Hash[Symbol, Integer]).void }
    def initialize(items:, metadata:) = super
  end

  # Single member
  SingleMember = Data.define(:id) do
    extend T::Sig

    sig { params(id: Integer).void }
    def initialize(id:) = super
  end

  # Many members (5+)
  ManyMembers = Data.define(:a, :b, :c, :d, :e) do
    extend T::Sig

    sig { params(a: Integer, b: String, c: Float, d: Symbol, e: T::Boolean).void }
    def initialize(a:, b:, c:, d:, e:) = super
  end
end

# ============================================================================
# Nested Data.define in module (common migration pattern)
# ============================================================================

module Predicates
  UserHasCheckouts = Data.define(:user) do
    extend T::Sig

    sig { params(user: String).void }
    def initialize(user:) = super

    sig { returns(T::Boolean) }
    def qualifies?
      !user.empty?
    end
  end
end

# ============================================================================
# Edge cases: block content variations
# ============================================================================

module EdgeCases
  # Data.define with block that has methods but no initialize
  DataWithOnlyMethods = Data.define(:x, :y) do
    def sum
      x + y
    end
  end

  # Data.define with initialize but WITHOUT sig (should remain untyped)
  UnsiggedInitialize = Data.define(:x, :y) do
    def initialize(x:, y:)
      super(x: x.to_i, y: y.to_i)
    end
  end

  # Data.define with initialize that is the only thing in the block
  OnlyInitialize = Data.define(:x) do
    def initialize(x:)
      super(x: x * 2)
    end
  end

  # Data.define with sig + initialize that has additional methods after
  TypedWithExtraMethods = Data.define(:name, :age) do
    extend T::Sig

    sig { params(name: String, age: Integer).void }
    def initialize(name:, age:) = super

    sig { returns(String) }
    def greeting
      "Hello, #{name}! You are #{age} years old."
    end

    sig { returns(T::Boolean) }
    def adult?
      age >= 18
    end
  end
end

# ============================================================================
# Typed Data.define with malformed or incomplete sigs
# Typed accessors are only created when precise constraints are met:
#   1. A sig with params(...) immediately precedes initialize
#   2. The initialize body is exactly bare `super`
# Otherwise Sorbet falls back to untyped or reports errors.
# ============================================================================

module MalformedSigs
  # Void sig without params — Sorbet reports a malformed sig error
  SigWithoutParams = Data.define(:x) do
    extend T::Sig

    sig { void }
    def initialize(x:) = super # error: Malformed `sig`. Type not specified for parameter `x`
  end

  # Partial params sig — omitting a member from the sig produces errors.
  # Sorbet enforces all-or-nothing: "Bad parameter ordering" and
  # "Malformed sig. Type not specified" are reported.

  # Sig with mismatched keyword names — Sorbet catches the mismatch
  MismatchedKeywordNames = Data.define(:x, :y) do
    extend T::Sig

    sig { params(x: Integer, z: String).void }
    def initialize(x:, z:) = super # error: Method `MalformedSigs::MismatchedKeywordNames#initialize` redefined with mismatched keyword argument name. Expected: `y`, got: `z`
  end

  # Non-void return type in initialize sig (unusual but syntactically valid)
  NonVoidReturnType = Data.define(:x) do
    extend T::Sig

    sig { params(x: Integer).returns(Integer) }
    def initialize(x:) = super
  end
end

# ============================================================================
# Typed Data.define with non-trivial initialize bodies
# When the initialize body is not bare `super`, types are NOT propagated
# to accessors — the members remain T.untyped.
# ============================================================================

module NonTrivialInitialize
  # Initialize with statements before super — not bare super
  StatementsBeforeSuper = Data.define(:x) do
    extend T::Sig

    sig { params(x: Integer).void }
    def initialize(x:)
      puts "creating"
      super
    end
  end

  # Initialize without any super call
  NoSuperCall = Data.define(:x) do
    extend T::Sig

    sig { params(x: Integer).void }
    def initialize(x:)
      # intentionally no super
    end
  end

  # Initialize with default keyword values — bare super, types propagate
  InitializeWithDefaults = Data.define(:name, :age) do
    extend T::Sig

    sig { params(name: String, age: Integer).void }
    def initialize(name:, age: 0) = super
  end
end

# ============================================================================
# Type checking validation
# ============================================================================

module TypeCheckTests
  Money = Data.define(:amount, :currency) do
    extend T::Sig

    sig { params(amount: Numeric, currency: String).void }
    def initialize(amount:, currency:) = super
  end

  def self.test_typed_readers
    m = Money.new(amount: 10, currency: "CAD")
    T.assert_type!(m.amount, Numeric)
    T.assert_type!(m.currency, String)
  end

  def self.test_wrong_type
    Money.new(amount: "10", currency: "CAD") # error: Expected `Numeric` but found `String("10")` for argument `amount`
  end

  def self.test_missing_kwarg
    Money.new(amount: 10) # error: Missing required keyword argument `currency` for method `TypeCheckTests::Money#initialize`
  end

  # Positional args are rejected when a typed initialize is provided.
  # Money.new(10, "CAD") would produce two errors:
  # - Too many positional arguments
  # - Missing required keyword arguments
end

module TypedEdgeCaseChecks
  def self.test_sig_without_params
    v = MalformedSigs::SigWithoutParams.new(x: "anything")
  end

  def self.test_initialize_with_defaults
    d = NonTrivialInitialize::InitializeWithDefaults.new(name: "Alice")
    T.assert_type!(d.name, String)
    T.assert_type!(d.age, Integer)
  end

  def self.test_typed_data_define
    g = ComplexTypes::SingleMember.new(id: 42)
    T.assert_type!(g.id, Integer)
  end

  def self.test_typed_data_define_wrong_type
    ComplexTypes::SingleMember.new(id: "not an int") # error: Expected `Integer` but found `String("not an int")` for argument `id`
  end
end

# ============================================================================
# Untyped Data.define still works as before
# ============================================================================

module UntypedTests
  Plain = Data.define(:x, :y)

  def self.test_untyped_accepts_anything
    p = Plain.new(1, 2)
    p.x
    p.y
  end
end
