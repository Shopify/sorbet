# RBS Support

## Problem building

Why do I need to comment `#require "rbs/collection"` in the rbs_parser gem?
    undefined method `current' for class Ractor


## Problems RBS

Require Ruby VM
No location on blocks?

## Unsupported RBS types

* Interface types (use bare class types instead)
* Alias types
* Literal types
* Boolish
* class -> T::Class[CurrentClass]
* self types on procs

# Generics types

We automatically translate `Array`, `Hash`, to their `T::` counterparts.


## TODO

* Const sigs? CONST = T.let(42, Integer)
* Pass `tc` on `rbi`

* Inline annotations? #: Type
* Pass `tc` on `rbi`

* Generic methods: Type variables
* Handle abstract, interface, final, sealed, mixes_in, required_ancestors
* Remove sorbet-runtime (t::Struct, t::Enum, etc...)

* Remove unnessary `to_s` calls?
* Clean how we depend on `rbs_parser`
* Remove rbs parser dependency on ruby_vm
