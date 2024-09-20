# typed: strict

class Foo
  extend T::Sig

  #: Integer?
  attr_writer :baz

  sig { returns(T.nilable(Integer)) }
  attr_accessor :bar

  #: -> void
  def initialize
    @baz = nil
    @bar = nil
  end
end
