# RBS Support

## Unsupported RBS types

* Interface types (use bare class types instead)
* Alias types
* Literal types
* Boolish



## TODO

// Test comments parsing
// test sigs (attributes, methods, const?)
// test errors

// - Handle types
    * Proc types
    * Record types
    * Tuple types
    * Type variales
    * class -> T::Class[CurrentClass]
    * Generics

// - Translate `rbi` gems signatures to RBS
    // - methods
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
    // - attributes

// - Handle other annotations
    // - Handle abstract, interface, final, sealed, mixes_in, overrides, required_ancestors

// - Handle errors

// - Pass `tc` on `rbi`
// - Clean how we depend on `rbs_parser`
// - Remove rbs parser dependency on ruby_vm
