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

# Generics types

We automatically translate `Array`, `Hash`, to their `T::` counterparts.


## TODO

* sig rewriter signature match errors + tests
* run before all other rewriters?
* Handle block self binding
* Generic methods: Type variales



* Remove unnessary `to_s` calls?

* attribute sigs + tests
* Const sigs?
* Inline annotations? #: Type
* Handle abstract, interface, final, sealed, mixes_in, overrides, required_ancestors

* Pass `tc` on `rbi`

* Clean how we depend on `rbs_parser`
* Remove rbs parser dependency on ruby_vm
