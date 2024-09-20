# typed: strict

class Foo
  #: Integer
  attr_reader :bar

  #: Integer?
  attr_accessor :baz

  #: Integer?
  attr_writer :qux

  #: (Integer) -> void
  def initialize(bar)
    @bar = bar
    @baz = 2
    @qux = 3
  end
end

foo = Foo.new(1)
T.reveal_type(foo.bar) # error: Revealed type: `Integer`
T.reveal_type(foo.baz) # error: Revealed type: `T.nilable(Integer)`
foo.qux = "" # error: Assigning a value to `qux` that does not match expected type `T.nilable(Integer)`
