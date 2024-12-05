# typed: strict

extend T::Sig

# class FooProc
#   #: (p: ^() [self: Integer] -> Integer ) ?{ (Integer) [self: FooProc] -> String } -> void
#   def initialize(p: -> { T.reveal_type(self); 42 }, &block)
#   end
# end

# FooProc.new do |foo|
#   T.reveal_type(self) # error: Revealed type: `FooProc`
#   "foo"
# end

#: -> ::Foo::Bar::Baz
def foo; end

# #: (String?) -> String
# def foo(x)
#   # if ARGV.first
#   #   puts x
#   #   return x #:: String
#   # end

#   # x #:: String
# end

# class Foo
#   def initialize
#     @foo = 42 #: Integer
#   end
# end

# # @abstract
# class C
#   # @abstract
#   #: (String) -> String
#   def foo(x); end
# end

# C.new.foo


# x.not
