# typed: strict

extend T::Sig

module A; end
module B; end
module C; end

module Foo
  class Bar
    class Baz; end
  end
end

# Class instance types

#: -> Foo
def class_instance_type1; T.unsafe(nil); end
T.reveal_type(class_instance_type1) # error: Revealed type: `Foo`

#: -> ::Foo
def class_instance_type2; T.unsafe(nil); end
T.reveal_type(class_instance_type2) # error: Revealed type: `Foo`

#: -> Foo::Bar
def class_instance_type3; T.unsafe(nil); end
T.reveal_type(class_instance_type3) # error: Revealed type: `Foo::Bar`

#: -> Foo::Bar::Baz
def class_instance_type4; T.unsafe(nil); end
T.reveal_type(class_instance_type4) # error: Revealed type: `Foo::Bar::Baz`

#: -> ::Foo::Bar
def class_instance_type5; T.unsafe(nil); end
T.reveal_type(class_instance_type5) # error: Revealed type: `Foo::Bar`

#: -> singleton(Foo)
def class_singleton_type1; T.unsafe(nil); end
T.reveal_type(class_singleton_type1) # error: Revealed type: `T.class_of(Foo)`

# Class singleton types

#: -> singleton(::Foo)
def class_singleton_type2; T.unsafe(nil); end
T.reveal_type(class_singleton_type2) # error: Revealed type: `T.class_of(Foo)`

#: -> singleton(Foo::Bar)
def class_singleton_type3; T.unsafe(nil); end
T.reveal_type(class_singleton_type3) # error: Revealed type: `T.class_of(Foo::Bar)`

#: -> singleton(Foo::Bar::Baz)
def class_singleton_type4; T.unsafe(nil); end
T.reveal_type(class_singleton_type4) # error: Revealed type: `T.class_of(Foo::Bar::Baz)`

#: -> singleton(::Foo::Bar)
def class_singleton_type5; T.unsafe(nil); end
T.reveal_type(class_singleton_type5) # error: Revealed type: `T.class_of(Foo::Bar)`

# Union types

#: -> (Foo | Foo::Bar)
def union_type1; T.unsafe(nil); end
T.reveal_type(union_type1) # error: Revealed type: `T.any(Foo, Foo::Bar)`

#: -> (Foo | Foo::Bar | ::Foo::Bar::Baz)
def union_type2; T.unsafe(nil); end
T.reveal_type(union_type2) # error: Revealed type: `T.any(Foo, Foo::Bar, Foo::Bar::Baz)`

# Intersection types

#: -> (A & B & C)
def intersection_type1; T.unsafe(nil); end
T.reveal_type(intersection_type1) # error: Revealed type: `T.all(A, B, C)`

# Optional types

#: -> Foo?
def optional_type1; T.unsafe(nil); end
T.reveal_type(optional_type1) # error: Revealed type: `T.nilable(Foo)`

# Base types

#: -> self
def base_type1; T.unsafe(nil); end
T.reveal_type(base_type1) # error: Revealed type: `T.class_of(<root>)`

class BaseType2
  #: -> instance
  def self.base_type2; T.unsafe(nil); end
end
T.reveal_type(BaseType2.base_type2) # error: Revealed type: `BaseType2`

# TODO: unsupported
#: -> class
def base_type3; T.unsafe(nil); end
T.reveal_type(base_type3) # error: Revealed type: `T.untyped`

#: -> bool
def base_type4; T.unsafe(nil); end
T.reveal_type(base_type4) # error: Revealed type: `T::Boolean`

#: -> nil
def base_type5; T.unsafe(nil); end
T.reveal_type(base_type5) # error: Revealed type: `NilClass`

#: -> top
def base_type6; T.unsafe(nil); end
T.reveal_type(base_type6) # error: Revealed type: `T.anything`

#: -> void
def base_type7; T.unsafe(nil); end
T.reveal_type(base_type7) # error: Revealed type: `Sorbet::Private::Static::Void`

# Generic types

#: -> Array[Integer]
def generic_type1; T.unsafe(nil); end
T.reveal_type(generic_type1) # error: Revealed type: `T::Array[Integer]`

#: -> Hash[String, Integer]
def generic_type2; T.unsafe(nil); end
T.reveal_type(generic_type2) # error: Revealed type: `T::Hash[String, Integer]`

#: -> T::Array[Integer]
def generic_type3; T.unsafe(nil); end
T.reveal_type(generic_type1) # error: Revealed type: `T::Array[Integer]`

#: -> T::Hash[String, Integer]
def generic_type4; T.unsafe(nil); end
T.reveal_type(generic_type2) # error: Revealed type: `T::Hash[String, Integer]`

class GenericType1
  extend T::Generic

  T1 = type_member
end

class GenericType2
  extend T::Generic

  T1 = type_member
  T2 = type_member
end

class GenericType3
  extend T::Generic

  T1 = type_member
  T2 = type_member
  T3 = type_member
end

#: -> GenericType1[Integer]
def generic_type5; T.unsafe(nil); end
T.reveal_type(generic_type5) # error: Revealed type: `GenericType1[Integer]`

#: -> GenericType2[GenericType1[untyped], GenericType3[Integer, String, untyped]]
def generic_type6; T.unsafe(nil); end
T.reveal_type(generic_type6) # error: Revealed type: `GenericType2[GenericType1[T.untyped], GenericType3[Integer, String, T.untyped]]`

# Tuples

#: -> [Integer]
def tuple_type1; T.unsafe(nil); end
T.reveal_type(tuple_type1) # error: Revealed type: `[Integer] (1-tuple)`

#: -> [Integer, String, untyped]
def tuple_type2; T.unsafe(nil); end
T.reveal_type(tuple_type2) # error: Revealed type: `[Integer, String, T.untyped] (3-tuple)`

# Shapes

#: -> { id: String, name: String }
def shape_type1; T.unsafe(nil); end
T.reveal_type(shape_type1) # error: Revealed type: `{id: String, name: String} (shape of T::Hash[T.untyped, T.untyped])`

# Proc types

#: -> ^(Integer, String) -> String
def proc_type1; T.unsafe(nil); end
T.reveal_type(proc_type1) # error: Revealed type: `T.proc.params(arg0: Integer, arg1: String).returns(String)`

# TODO
# proc_type -> Proc
  # self binding

# mixed tests

# Translates to `T.noreturn`
#: -> bot
def base_type_last; T.unsafe(nil); end
T.reveal_type(base_type_last) # error: This code is unreachable