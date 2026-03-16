# typed: strict
# enable-experimental-rbs-comments: true
# Modified for Prism: csend desugaring assigns inner/outer temp variables in
# opposite order ($3/$4, $5/$6 swapped), producing semantically equivalent but
# textually different rewrite-tree output.

# let

class Let
  #: (*untyped) -> void
  def foo=(*args); end

  #: -> untyped
  def foo; end
end

let1 = Let.new #: Let?
let1&.foo = "foo" #: String

let2 = Let.new #: Let?
let2&.foo&.foo = "foo" #: String

let3 = Let.new #: Let?
let3&.foo&.foo = "foo", "bar" #: Array[String]
