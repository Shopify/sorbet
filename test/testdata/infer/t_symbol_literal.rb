# typed: true

s = :abc
T.assert_type!(s, T::Symbol(:abc))

s2 = :xyz
T.assert_type!(s2, T::Symbol(:abc)) # error: does not have asserted type `Symbol(:abc)`

extend T::Sig

sig {params(x: T::Symbol(:foo)).void}
def takes_foo(x); end

takes_foo(:foo)
takes_foo(:bar) # error: Expected `Symbol(:foo)` but found `Symbol(:bar)`

# Joining different symbol literals across branches should produce a union
coin_flip = T.unsafe(nil)
x = if coin_flip
  :heads
else
  :tails
end
T.reveal_type(x) # error: Revealed type: `T.any(Symbol(:heads), Symbol(:tails))`

# Bare symbol literals in type syntax
sig {params(x: :foo).void}
def takes_foo_bare(x); end

takes_foo_bare(:foo)
takes_foo_bare(:bar) # error: Expected `Symbol(:foo)` but found `Symbol(:bar)`

sig {returns(:ok)}
def returns_ok
  :ok
end

T.assert_type!(returns_ok, T::Symbol(:ok))

# Bare symbols in T.any
y = T.let(:heads, T.any(:heads, :tails))
T.reveal_type(y) # error: Revealed type: `T.any(Symbol(:heads), Symbol(:tails))`

# Bare symbol in T.let
z = T.let(:foo, :foo)
T.reveal_type(z) # error: Revealed type: `Symbol(:foo)`

# Bare symbol in T.nilable
w = T.let(nil, T.nilable(:foo))
T.reveal_type(w) # error: Revealed type: `T.nilable(Symbol(:foo))`
