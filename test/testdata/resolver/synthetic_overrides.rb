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

module OtherInterface
  extend T::Sig
  extend T::Helpers

  interface!

  sig { abstract.params(val: String).returns(String) }
  def bar=(val); end

  sig { abstract.returns(String) }
  def bar; end
end

class BadProp < T::Struct
  include Interface
  include OtherInterface

  prop :foo, String, override: :getter
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ error: Return type `String` does not match return type of abstract method `Interface#foo`

  prop :bar, Integer, override: :both
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ error: Return type `Integer` does not match return type of abstract method `OtherInterface#bar`
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ error: Parameter `bar` of type `Integer` not compatible with type of abstract method `OtherInterface#bar=`
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ error: Return type `Integer` does not match return type of abstract method `OtherInterface#bar=`
end

class GoodProp < T::Struct
  include Interface
  include OtherInterface

  prop :foo, Integer, override: :getter
  prop :bar, String, override: :both
end

class IncompatibleProp < T::Struct
  include Interface
  include OtherInterface

  prop :foo, String, override: :getter, allow_incompatible: true

  prop :bar, Integer, override: :both, allow_incompatible: true
end

class MissingOverrideProp < T::Struct
  include Interface
  include OtherInterface

  prop :foo, Integer
# ^^^^^^^^^^^^^^^^^^ error: Method `MissingOverrideProp#foo` implements an abstract method `Interface#foo` but is not declared with `override.`

  prop :bar, String
# ^^^^^^^^^^^^^^^^^ error: Method `MissingOverrideProp#bar=` implements an abstract method `OtherInterface#bar=` but is not declared with `override.`
# ^^^^^^^^^^^^^^^^^ error: Method `MissingOverrideProp#bar` implements an abstract method `OtherInterface#bar` but is not declared with `override.`
end

class WrongOverrideValueProp < T::Struct
  include Interface

  prop :foo, Integer, override: true
                              # ^^^^ error: The valid values for a prop `override` are `:both`, `:getter` or `false`
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ error: Method `WrongOverrideValueProp#foo` implements an abstract method `Interface#foo` but is not declared with `override.`
end
