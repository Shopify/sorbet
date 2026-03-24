# typed: true
# enable-experimental-rbs-comments: true

# ============================================================================
# Basic typed members
# ============================================================================

TypedPoint = Data.define(
  :x, #: Integer
  :y, #: Integer
)

point = TypedPoint.new(x: 1, y: 2)
T.reveal_type(point.x) # error: Revealed type: `Integer`
T.reveal_type(point.y) # error: Revealed type: `Integer`
T.assert_type!(point.x, Integer)
T.assert_type!(point.y, Integer)

# Wrong type errors
TypedPoint.new(x: "bad", y: 2) # error: Expected `Integer` but found `String("bad")`

# Missing required keyword argument
TypedPoint.new(x: 1) # error: Missing required keyword argument `y`

# Extra keyword argument
TypedPoint.new(x: 1, y: 2, z: 3) # error: Unrecognized keyword argument `z`

# ============================================================================
# Complex types (RBS syntax)
# ============================================================================

TypedMoney = Data.define(
  :amount, #: Numeric
  :currency, #: String
)

money = TypedMoney.new(amount: 1.0, currency: "USD")
T.assert_type!(money.amount, Numeric)
T.assert_type!(money.currency, String)

# Nilable types
NilableData = Data.define(
  :name, #: String?
  :age, #: Integer?
)

nilable = NilableData.new(name: nil, age: nil)
T.reveal_type(nilable.name) # error: Revealed type: `T.nilable(String)`
T.reveal_type(nilable.age) # error: Revealed type: `T.nilable(Integer)`

# Union types
UnionData = Data.define(
  :value, #: Integer | String
)

union = UnionData.new(value: 42)
T.reveal_type(union.value) # error: Revealed type: `T.any(Integer, String)`

# Generic collection types
CollectionData = Data.define(
  :items, #: Array[String]
  :lookup, #: Hash[Symbol, Integer]
)

generic = CollectionData.new(items: ["a"], lookup: {foo: 1})
T.reveal_type(generic.items) # error: Revealed type: `T::Array[String]`
T.reveal_type(generic.lookup) # error: Revealed type: `T::Hash[Symbol, Integer]`

# Boolean type
BoolData = Data.define(
  :flag, #: bool
)

bool_data = BoolData.new(flag: true)
T.reveal_type(bool_data.flag) # error: Revealed type: `T::Boolean`

# ============================================================================
# Partially typed (y is T.untyped)
# ============================================================================

PartiallyTyped = Data.define(
  :x, #: Integer
  :y,
)

partial = PartiallyTyped.new(x: 1, y: "anything")
T.reveal_type(partial.x) # error: Revealed type: `Integer`
T.reveal_type(partial.y) # error: Revealed type: `T.untyped`

# ============================================================================
# Block with additional methods
# ============================================================================

DataWithMethods = Data.define(
  :name, #: String
  :age, #: Integer
) do
  #: -> String
  def greeting = "Hello #{name}, age #{age}"
end

obj = DataWithMethods.new(name: "Alice", age: 30)
T.assert_type!(obj.name, String)
T.assert_type!(obj.age, Integer)
T.reveal_type(obj.greeting) # error: Revealed type: `String`

# ============================================================================
# Fully qualified ::Data.define
# ============================================================================

QualifiedData = ::Data.define(
  :value, #: Float
)

qd = QualifiedData.new(value: 1.0)
T.assert_type!(qd.value, Float)

# ============================================================================
# Single member
# ============================================================================

SingleMember = Data.define(
  :id, #: Integer
)

single = SingleMember.new(id: 42)
T.assert_type!(single.id, Integer)

# ============================================================================
# Many members (5+)
# ============================================================================

ManyMembers = Data.define(
  :a, #: Integer
  :b, #: String
  :c, #: Float
  :d, #: Symbol
  :e, #: bool
)

many = ManyMembers.new(a: 1, b: "hi", c: 1.0, d: :sym, e: true)
T.assert_type!(many.a, Integer)
T.assert_type!(many.b, String)
T.assert_type!(many.c, Float)
T.assert_type!(many.d, Symbol)
T.assert_type!(many.e, T::Boolean)

# ============================================================================
# Nested in modules
# ============================================================================

module Payments
  Invoice = Data.define(
    :id, #: Integer
    :total, #: Numeric
  )

  inv = Invoice.new(id: 1, total: 99.99)
  T.assert_type!(inv.id, Integer)
  T.assert_type!(inv.total, Numeric)

  Invoice.new(id: "bad", total: 1) # error: Expected `Integer` but found `String("bad")`
end

# ============================================================================
# Malformed type comment — falls back to T.untyped without crashing
# ============================================================================

MalformedType = Data.define(
  :x, #: !!!  # error: Failed to parse RBS type
  :y, #: Integer
)

# x should degrade to T.untyped due to parse failure
mal = MalformedType.new(x: "anything", y: 42)
T.reveal_type(mal.x) # error: Revealed type: `T.untyped`
T.reveal_type(mal.y) # error: Revealed type: `Integer`
