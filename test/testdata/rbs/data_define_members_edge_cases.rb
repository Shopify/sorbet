# typed: true
# enable-experimental-rbs-comments: true

# ============================================================================
# No typed members — should not add anything extra, members stay untyped
# ============================================================================

UntypedData = Data.define(:x, :y)

untyped = UntypedData.new(x: 1, y: 2)
T.reveal_type(untyped.x) # error: Revealed type: `T.untyped`
T.reveal_type(untyped.y) # error: Revealed type: `T.untyped`

# ============================================================================
# Explicit initialize with sig in block takes precedence over inline types
# ============================================================================

MoneyWithDefault = Data.define(
  :amount, #: Numeric
  :currency, #: String
) do
  #: (amount: Numeric, ?currency: String) -> void
  def initialize(amount:, currency: "USD") = super
end

mwd = MoneyWithDefault.new(amount: 10)
T.assert_type!(mwd.amount, Numeric)
T.assert_type!(mwd.currency, String)

# ============================================================================
# Block with methods + inline types (no explicit initialize) — types work
# ============================================================================

DataWithMethodsAndTypes = Data.define(
  :x, #: Integer
  :y, #: String
) do
  #: -> String
  def to_s = "#{x},#{y}"
end

dmt = DataWithMethodsAndTypes.new(x: 1, y: "hello")
T.reveal_type(dmt.x) # error: Revealed type: `Integer`
T.reveal_type(dmt.y) # error: Revealed type: `String`

# ============================================================================
# Block with methods but no typed members — members stay untyped
# ============================================================================

DataWithMethodsOnly = Data.define(:x, :y) do
  #: -> String
  def to_s = "#{x},#{y}"
end

dmo = DataWithMethodsOnly.new(x: 1, y: 2)
T.reveal_type(dmo.x) # error: Revealed type: `T.untyped`
T.reveal_type(dmo.y) # error: Revealed type: `T.untyped`

# ============================================================================
# Empty block with typed members
# ============================================================================

EmptyBlockTyped = Data.define(
  :x, #: Integer
) do
end

ebt = EmptyBlockTyped.new(x: 42)
T.reveal_type(ebt.x) # error: Revealed type: `Integer`

# ============================================================================
# Scoped Data class (Foo::Data) — should NOT be rewritten
# ============================================================================

module Foo
  class Data
    def self.define(*args); end
  end
end

NotRealData = Foo::Data.define(
  :x, #: Integer # error: Argument does not have asserted type `Integer`
)

# ============================================================================
# Interaction with traditional Sorbet sig annotations
# ============================================================================

# Traditional sig + bare-super initialize (no inline types) works via Data.cc
SorbetMoney = Data.define(:amount, :currency) do
  extend T::Sig

  sig { params(amount: Numeric, currency: String).void }
  def initialize(amount:, currency:) = super
end

sm = SorbetMoney.new(amount: 10, currency: "CAD")
T.assert_type!(sm.amount, Numeric)
T.assert_type!(sm.currency, String)
SorbetMoney.new(amount: "bad", currency: "CAD") # error: Expected `Numeric` but found `String("bad")`

# Sorbet sig + initialize WITH inline types — explicit Sorbet sig takes precedence
BothSigStyles = Data.define(
  :x, #: Integer
  :y, #: String
) do
  extend T::Sig

  sig { params(x: Numeric, y: String).void }
  def initialize(x:, y:) = super
end

# Sorbet sig's Numeric takes precedence over inline #: Integer
bs = BothSigStyles.new(x: 1, y: "hi")
T.assert_type!(bs.x, Numeric)
T.assert_type!(bs.y, String)

# RBS method sig + initialize WITH inline types — explicit RBS sig takes precedence
RbsBothStyles = Data.define(
  :x, #: Integer
  :y, #: String
) do
  #: (x: Numeric, y: String) -> void
  def initialize(x:, y:) = super
end

# RBS sig's Numeric takes precedence over inline #: Integer
rbs = RbsBothStyles.new(x: 1, y: "hi")
T.assert_type!(rbs.x, Numeric)
T.assert_type!(rbs.y, String)

# Sorbet sig + non-bare-super — readers fall back to untyped even with sig
CoercedData = Data.define(:amount, :currency) do
  extend T::Sig

  sig { params(amount: Numeric, currency: String).void }
  def initialize(amount:, currency:)
    super(amount: amount.to_i, currency: currency)
  end
end

cd = CoercedData.new(amount: 10, currency: "CAD")
T.reveal_type(cd.amount) # error: Revealed type: `T.untyped`
T.reveal_type(cd.currency) # error: Revealed type: `T.untyped`
# The sig still constrains the constructor even though readers are untyped
CoercedData.new(amount: :bad, currency: "CAD") # error: Expected `Numeric` but found `Symbol(:bad)`
