# typed: strict

module Minitest
  class Test; end
end

class BestCaseTestClass < Minitest::Test
  extend T::Sig

  sig { params(a: T.untyped, b: T.untyped).void }
  def assert_equal(a, b); end

  sig { void }
  def initialize
    @a = T.let(3, Integer)
  end

  sig { void }
  def setup
    @b = T.let(4, Integer)
  end

  test "it works" do
    assert_equal 3, @a
    assert_equal 4, @b
  end
end

class SubclassOfMinitestTest < Minitest::Test
end

class SubSubclassOfMinitestTest < SubclassOfMinitestTest
  extend T::Sig

  sig { params(a: T.untyped, b: T.untyped).void }
  def assert_equal(a, b); end

  sig { void }
  def initialize
    @x = T.let(1, Integer)
  end

  sig { void }
  def setup
    @y = T.let(2, Integer)
  end

  test "it works" do
    assert_equal 1, @x
    assert_equal 2, @y
  end
end
