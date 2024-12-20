# typed: strict
# frozen_string_literal: true

class Foo
  extend T::Sig

  sig { returns({"name" => String, "amount" => Integer}) }
  def test
    { "name" => "Seth", "amount" => 100 }
  end

  #: -> {"a" => String, "b" => Integer}
  def foo
    { "a" => "Foo", "b" => 42 }
  end

  #: -> {a: String, b: Integer}
  def bar
    { a: "Foo", b: 42 }
  end

  #: -> {"a" => String, :b => Integer}
  def baz
    { "a" => "Foo", b: 42 }
  end
end
