# typed: true
# enable-experimental-boolish-type: true

extend T::Sig

sig { params(b: T::Boolish).void }
def uses_boolish(b)
  if b
    puts "truthy"
  end

  unless b
    puts "falsy"
  end

  !b

  b && true
  b || false
end

sig { params(b: T::Boolish).void }
def forbidden_methods(b)
  b.to_s
  # ^^^^ error: Method `to_s` does not exist on `T::Boolish`

  b.class
  # ^^^^^ error: Method `class` does not exist on `T::Boolish`

  b == true
  # ^^ error: Method `==` does not exist on `T::Boolish`

  b.equal?(true)
  # ^^^^^^ error: Method `equal?` does not exist on `T::Boolish`

  b & true
  # ^ error: Method `&` does not exist on `T::Boolish`

  b | false
  # ^ error: Method `|` does not exist on `T::Boolish`
end

sig { returns(T::Boolish) }
def returns_true
  true
end

sig { returns(T::Boolish) }
def returns_false
  false
end

sig { returns(T::Boolish) }
def returns_nil
  nil
end
