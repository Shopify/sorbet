# typed: true
# enable-experimental-rbs-comments: true

# Basic typed members via virtual initialize
TypedPoint = Data.define(:x, :y) do
  #: (x: Integer, y: String) -> void
end

TypedPoint.new(x: 1, y: "hello")
TypedPoint.new(x: "bad", y: "hello") # error: Expected `Integer` but found `String("bad")` for argument `x`
T.reveal_type(TypedPoint.new(x: 1, y: "hi").x) # error: Revealed type: `Integer`
T.reveal_type(TypedPoint.new(x: 1, y: "hi").y) # error: Revealed type: `String`

# With explicit initialize and defaults
Money = Data.define(:amount, :currency) do
  #: (amount: Numeric, ?currency: String) -> void
  def initialize(amount:, currency: "USD") = super
end
T.reveal_type(Money.new(amount: 10).currency) # error: Revealed type: `String`
T.reveal_type(Money.new(amount: 10).amount) # error: Revealed type: `Numeric`
Money.new(amount: "bad") # error: Expected `Numeric` but found `String("bad")` for argument `amount`

# With additional methods in block
DataWithMethods = Data.define(:user) do
  #: (user: String) -> void

  #: -> T::Boolean
  def active?
    !user.empty?
  end
end

T.reveal_type(DataWithMethods.new(user: "alice").user) # error: Revealed type: `String`

# Fully qualified ::Data
QualifiedData = ::Data.define(:a, :b) do
  #: (a: Integer, b: Float) -> void
end
T.reveal_type(QualifiedData.new(a: 1, b: 2.0).a) # error: Revealed type: `Integer`
T.reveal_type(QualifiedData.new(a: 1, b: 2.0).b) # error: Revealed type: `Float`

# Untyped fallback (no sig) - existing behavior
PlainData = Data.define(:x, :y)
T.reveal_type(PlainData.new(1, 2).x) # error: Revealed type: `T.untyped`

# Block with no sig - existing behavior
BlockNoSig = Data.define(:x) do
  #: -> String
  def to_s
    "BlockNoSig"
  end
end
T.reveal_type(BlockNoSig.new(1).x) # error: Revealed type: `T.untyped`

# Mismatched sig param names vs Data.define members - falls back to untyped.
# The synthesized def initialize(x:) doesn't match the sig param (a:),
# so the RBS translator produces parameter mismatch errors and the sig is discarded.
MismatchedSingle = Data.define(:x) do # error: Malformed `sig`. Type not specified for parameter `x`
  #: (a: Integer) -> void
  #   ^ error: Unknown parameter name `a`
end
T.reveal_type(MismatchedSingle.new(x: 1).x) # error: Revealed type: `T.untyped`

# Partial member typing - only some members have types
PartialData = Data.define(:x, :y, :z) do
  #: (x: Integer, y: String, z: Float) -> void
end
T.reveal_type(PartialData.new(x: 1, y: "hi", z: 2.0).x) # error: Revealed type: `Integer`
T.reveal_type(PartialData.new(x: 1, y: "hi", z: 2.0).y) # error: Revealed type: `String`
T.reveal_type(PartialData.new(x: 1, y: "hi", z: 2.0).z) # error: Revealed type: `Float`

# Block with non-method statements and orphan sig (e.g. include)
module Printable; end
IncludeData = Data.define(:val) do
  #: (val: Integer) -> void

  include Printable
end
T.reveal_type(IncludeData.new(val: 42).val) # error: Revealed type: `Integer`

# Complex types: T.nilable
NilableData = Data.define(:name, :email) do
  #: (name: String, email: String?) -> void
end
T.reveal_type(NilableData.new(name: "alice", email: nil).name) # error: Revealed type: `String`
T.reveal_type(NilableData.new(name: "alice", email: nil).email) # error: Revealed type: `T.nilable(String)`

# Complex types: T.any union
UnionData = Data.define(:value) do
  #: (value: String | Integer | Symbol) -> void
end
T.reveal_type(UnionData.new(value: "hi").value) # error: Revealed type: `T.any(String, Integer, Symbol)`

# Complex types: generic collections
CollectionData = Data.define(:items, :meta) do
  #: (items: Array[String], meta: Hash[Symbol, Integer]) -> void
end
T.reveal_type(CollectionData.new(items: ["a"], meta: {x: 1}).items) # error: Revealed type: `T::Array[String]`
T.reveal_type(CollectionData.new(items: ["a"], meta: {x: 1}).meta) # error: Revealed type: `T::Hash[Symbol, Integer]`

# Many members (5+)
ManyMembers = Data.define(:a, :b, :c, :d, :e) do
  #: (a: Integer, b: String, c: Float, d: Symbol, e: bool) -> void
end
T.reveal_type(ManyMembers.new(a: 1, b: "x", c: 1.0, d: :y, e: true).a) # error: Revealed type: `Integer`
T.reveal_type(ManyMembers.new(a: 1, b: "x", c: 1.0, d: :y, e: true).e) # error: Revealed type: `T::Boolean`

# Nested in module
module Predicates
  UserCheck = Data.define(:user) do
    #: (user: String) -> void

    #: -> bool
    def qualifies?
      !user.empty?
    end
  end
end
T.reveal_type(Predicates::UserCheck.new(user: "alice").user) # error: Revealed type: `String`

# Type checking: wrong type produces error
WrongType = Data.define(:x) do
  #: (x: Integer) -> void
end
WrongType.new(x: "bad") # error: Expected `Integer` but found `String("bad")` for argument `x`

# Type checking: missing kwarg produces error
MissingKwarg = Data.define(:x, :y) do
  #: (x: Integer, y: String) -> void
end
MissingKwarg.new(x: 1) # error: Missing required keyword argument `y` for method `MissingKwarg#initialize`

# Explicit initialize with RBS sig (not virtual — sig attaches to the def directly)
RbsExplicit = Data.define(:amount, :currency) do
  #: (amount: Numeric, currency: String) -> void
  def initialize(amount:, currency:) = super
end
T.assert_type!(RbsExplicit.new(amount: 10, currency: "CAD").amount, Numeric)
T.assert_type!(RbsExplicit.new(amount: 10, currency: "CAD").currency, String)
RbsExplicit.new(amount: "bad", currency: "CAD") # error: Expected `Numeric` but found `String("bad")` for argument `amount`
