# typed: strict
# enable-experimental-rbs-signatures: true

# @abstract
class Foo
  # @abstract

  # @abstract: # error: Failed to parse RBS member definition (unexpected token for method name)

  # @abstract: e1: -> String
  #              ^ error: Failed to parse RBS member definition (unexpected token for method name)

  # @abstract: def e2:
  #            ^^^^^^^ error: Failed to parse RBS member definition (unexpected token for method type)

  # @abstract: def e3: -> String | -> Integer
  #                              ^^^^^^^^^^^^ error: RBS signatures for abstract methods cannot have overloads

  # @abstract: def e4: (Integer) -> void
  #                     ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed positional parameters

  # @abstract: def e5: (*Integer) -> String
  #                      ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed positional parameters

  # @abstract: def e6: (**Integer) -> String
  #                       ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed keyword parameters

  # @abstract: def e8: (Integer a) -> String | ...
  #                                            ^^^ error: Failed to parse RBS member definition (unexpected overloading method definition)

  # @abstract: def m1: (Integer a) -> String

  # @abstract: def m2: (Integer a, ?String b, *Integer c, d: String, ?e: Integer, **Integer f) { -> void } -> String

  # @abstract: private def m3: () -> String
end

class Bar < Foo
  # @override
  #: (Integer a) -> String
  def m1(a)
    "bar"
  end

  # @override
  #: (Integer a, ?String b, *Integer c, d: String, ?e: Integer, **Integer f) { -> void } -> String
  def m2(a, b = "", *c, d:, e: 1, **f, &g)
    "bar"
  end

  # @override
  #: () -> String
  private def m3
    "bar"
  end
end

# @abstract
class Baz
  # @abstract: def foo: (Integer a) -> String

  #: -> String
  def bar
    foo(42)
  end
end

class Qux
  # @abstract: def foo: -> void
  #            ^^^^^^^^^^^^^^^^ error: Before declaring an abstract method, you must mark your class/module as abstract using `abstract!` or `interface!`
end
