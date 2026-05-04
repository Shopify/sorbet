# typed: true
# enable-experimental-boolish-type: true

extend T::Sig

sig { params(x: T::Trueish).void }
def takes_trueish(x); end

sig { params(x: T::Falseish).void }
def takes_falseish(x); end

sig { params(b: T::Boolish).void }
def narrow(b)
  if b
    takes_trueish(b)
    takes_falseish(b)
    #             ^ error: Expected `T::Falseish` but found `T::Trueish` for argument `x`
  else
    takes_falseish(b)
    takes_trueish(b)
    #             ^ error: Expected `T::Trueish` but found `T::Falseish` for argument `x`
  end
end

sig { params(b: T::Boolish).void }
def narrow_with_not(b)
  if !b
    takes_falseish(b)
    takes_trueish(b)
    #             ^ error: Expected `T::Trueish` but found `T::Falseish` for argument `x`
  else
    takes_trueish(b)
    takes_falseish(b)
    #             ^ error: Expected `T::Falseish` but found `T::Trueish` for argument `x`
  end
end
