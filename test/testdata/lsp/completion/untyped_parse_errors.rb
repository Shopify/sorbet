# typed: false
# disable-parser-comparison: true

class A
  e.
  # ^ completion: (nothing)
end # error: unexpected token
