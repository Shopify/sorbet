# typed: true

extend T::Sig

#: (Integer) -> String
def foo(x)
  (x + 1).to_s
end

x = foo("foo")
T.reveal_type(x)
