# typed: strict
# enable-experimental-rbs-signatures: true

# @abstract
class Foo
  # @abstract
  #  ^^^^^^^^ error: RBS signatures for abstract methods must be formatted as `# @abstract: def name: () -> void`
  #: -> void
  def e1; end

  # @abstract: # error: Failed to parse RBS member definition (unexpected token for method name)

  # @abstract: e2: -> String
  #              ^ error: Failed to parse RBS member definition (unexpected token for method name)

  # @abstract: def e3:
  #            ^^^^^^^ error: Failed to parse RBS member definition (unexpected token for method type)

  # @abstract: def e4: -> String | -> Integer
  #                              ^^^^^^^^^^^^ error: RBS signatures for abstract methods cannot have overloads

  # @abstract: def e5: (Integer) -> void
  #                     ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed positional parameters

  # @abstract: def e6: (*Integer) -> String
  #                      ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed positional parameters

  # @abstract: def e7: (**Integer) -> String
  #                       ^^^^^^^ error: RBS signatures for abstract methods cannot have unnamed keyword parameters

  # @abstract: def e8: (Integer a) -> String | ...
  #                                            ^^^ error: Failed to parse RBS member definition (unexpected overloading method definition)

  # @abstract: def m1: (Integer a) -> String

  # @abstract: def m2: (Integer a, ?String b, *Integer c, d: String, ?e: Integer, **Integer f) { -> void } -> String

  # @abstract: private def m3: () -> String

  # @abstract: def self.m4: -> String
end

class Bar < Foo # error: Missing definition for abstract method `Foo.m4` in `T.class_of(Bar)`
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
