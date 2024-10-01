# typed: strict

class Foo
  extend T::Sig

  sig { void }
  def initialize
    @x = ARGV.first #: String # This is a test
    T.reveal_type(@x)

    @y = nil #: Integer? # This is a test
    T.reveal_type(@y)
  end
end
