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

# proc_type -> Proc
# self binding
# tuple
# shape

# mixed tests

# Translates to `T.noreturn`
#: -> bot
def base_type_last; T.unsafe(nil); end
T.reveal_type(base_type_last) # error: This code is unreachable
