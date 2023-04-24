# typed: true

class Foo
  extend T::Sig
  extend T::Helpers

  def foo
    42
  end

  def method_missing(name)
    Bar.new.send(name)
  end

  delegates_missing_methods_to { Bar }
end

class Bar
  def bar
    12
  end
end

foo = Foo.new
# no Sorbet error
foo.foo #=> 42
# no Sorbet error
foo.bar #=> 12
