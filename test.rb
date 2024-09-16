# typed: true

extend T::Sig

#: (Integer) -> String
def foo(x)
  x + 1
end

x = foo("foo")
T.reveal_type(x)
