# typed: true

module Interface
  extend T::Sig
  extend T::Helpers

  interface!

  sig { abstract.returns(Integer) }
  def foo; end
end

class BadAttr
  extend T::Sig
  include Interface

  sig { override.returns(String) }
  attr_reader :foo
# ^^^^^^^^^^^^^^^^ error: Return type `String` does not match return type of abstract method `Interface#foo`

  def initialize
    @foo = "string"
  end
end

class GoodAttr
  extend T::Sig
  include Interface

  sig { override.returns(Integer) }
  attr_reader :foo

  def initialize
    @foo = 321
  end
end

# TODO: implement override checks for props only static checker

class BadProp < T::Struct
  include Interface

  prop :foo, String
end

class GoodProp < T::Struct
  include Interface

  prop :foo, Integer
end
