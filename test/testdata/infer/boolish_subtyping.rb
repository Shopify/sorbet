# typed: true
# enable-experimental-boolish-type: true

extend T::Sig

sig { params(x: T::Trueish).void }
def takes_trueish(x); end

sig { params(x: T::Falseish).void }
def takes_falseish(x); end

sig { params(x: T::Boolish).void }
def takes_boolish(x); end

# Trueish accepts true literal and TrueClass values
takes_trueish(true)

sig { params(t: TrueClass).void }
def from_true(t)
  takes_trueish(t)
end

# Trueish rejects false / nil / FalseClass
takes_trueish(false)
#             ^^^^^ error: Expected `T::Trueish` but found `FalseClass(false)` for argument `x`
takes_trueish(nil)
#             ^^^ error: Expected `T::Trueish` but found `NilClass` for argument `x`

sig { params(f: FalseClass).void }
def from_false(f)
  takes_trueish(f)
  #             ^ error: Expected `T::Trueish` but found `FalseClass` for argument `x`
end

# Falseish accepts false, nil, FalseClass, NilClass
takes_falseish(false)
takes_falseish(nil)

sig { params(f: FalseClass, n: NilClass).void }
def from_falsy(f, n)
  takes_falseish(f)
  takes_falseish(n)
end

# Falseish rejects true / TrueClass
takes_falseish(true)
#              ^^^^ error: Expected `T::Falseish` but found `TrueClass(true)` for argument `x`

sig { params(t: TrueClass).void }
def from_true_to_falseish(t)
  takes_falseish(t)
  #              ^ error: Expected `T::Falseish` but found `TrueClass` for argument `x`
end

# Boolish accepts all of true / false / nil
takes_boolish(true)
takes_boolish(false)
takes_boolish(nil)
