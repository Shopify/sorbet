# typed: true
# disable-parser-comparison: true

class A
  attr_accessor :foo
end

def main
  a = A.new
  a.
  # ^ completion: foo, foo=, ...
  # ^ apply-completion: [A] item: 1
end # error: unexpected token "end"
