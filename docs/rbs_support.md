# RBS Support

## Unsupported RBS types

* Interface types (use bare class types instead)
* Alias types
* Literal types
* Boolish
* class -> T::Class[CurrentClass]

# Generics types

We automatically translate `Array`, `Hash`, to their `T::` counterparts.


## TODO

* Handle rewriter errors
  * test errors

* Translate `rbi` gems signatures to RBS for methods
  * tests method sigs
  * Handle block self binding
  * Generic methods: Type variales

* Test comments parsing + errors
* attribute sigs + tests

* Remove unnessary `to_s` calls?
* Const sigs?
* Inline annotations? #: Type
* Handle abstract, interface, final, sealed, mixes_in, overrides, required_ancestors

* Pass `tc` on `rbi`

* Clean how we depend on `rbs_parser`
* Remove rbs parser dependency on ruby_vm
