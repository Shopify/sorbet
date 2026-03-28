# typed: strict
# enable-experimental-rbs-comments: true

extend T::Sig

# Symbol literal return type
#: () -> :foo
def symbol_literal_return; :foo; end

T.reveal_type(symbol_literal_return) # error: Revealed type: `Symbol(:foo)`

# Symbol literal parameter type
#: (:foo) -> void
def symbol_literal_param(x); end

# Symbol literal union
#: (:heads | :tails) -> void
def symbol_literal_union(x); end

symbol_literal_union(:heads)
symbol_literal_union(:other) # error: Expected `T.any(Symbol(:heads), Symbol(:tails))` but found `Symbol(:other)`

# Non-symbol literals are still unsupported
#: () -> "foo"
#        ^^^^^ error: RBS literal types are not supported
def string_literal; T.unsafe(nil); end

#: () -> 42
#        ^^ error: RBS literal types are not supported
def integer_literal; T.unsafe(nil); end
