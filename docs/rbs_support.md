# RBS Support

## Unsupported RBS types

* Interface types (use bare class types instead)
* Alias types
* Literal types
* Boolish

# Generics types

We automatically translate `Array`, `Hash`, to their `T::` counterparts.


## TODO

// Test comments parsing
// test sigs (attributes, methods, const?)

// - Translate `rbi` gems signatures to RBS for methods
    // - Handle required positional args
    // - Handle optional positional args
    // - Handle required keyword args
    // - Handle optional keyword args
    // - Handle rest args
    // - Handle keyword rest args
    // - Handle block args
    // - Handle type params
    // - Handle return types
    // - Handle block binding
    // - tests

// - Handle types
    * Proc types: Self binding
    * Generic methods: Type variales

    * class -> T::Class[CurrentClass]

// - Remove unnessary `to_s` calls?
// - Translate attributes
// - Handle errors
    // test errors
// - Handle abstract, interface, final, sealed, mixes_in, overrides, required_ancestors


// - Pass `tc` on `rbi`
// - Clean how we depend on `rbs_parser`
// - Remove rbs parser dependency on ruby_vm
