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
