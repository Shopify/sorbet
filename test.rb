# typed: true

class Foo
  # Some documentation
  #: (String) -> Integer
  def foo(x)
    x
  end
end

# #: (String) -> String
# def bar(x)
#   y = x #: Integer
#   y
# end

Foo.new.foo(42)
