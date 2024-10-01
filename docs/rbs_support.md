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

* translate as Array, Hash, etc...
* push fixes to `rbi`
* push translate to `spoom` (--sigs, --attrs, --consts, --methods)
    spoom rbs translate-rbi ? or use `bundle exec rbi`?

* Const sigs? CONST = T.let(42, Integer)
* Generic classes: Type arguments
* Generic methods: Type variables
* Handle mixes_in, required_ancestors

* Collect comments and associate them to the correct nodes
* Remove unnessary `to_s` calls?
* Clean how we depend on `rbs_parser`
* Remove rbs parser dependency on ruby_vm

