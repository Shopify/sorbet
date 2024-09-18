# typed: true

#: (?String x) -> String
def foo(x)
  T.reveal_type(x) # error: Revealed type: `String`
end
