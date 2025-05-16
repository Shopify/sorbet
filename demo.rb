# typed: true
extend T::Sig

sig { returns(T.nilable(Integer)) }
def returns_nilable()
  nil
end

sig { params(i: Integer).returns(Integer) }
def takes_int(i)
  i
end

sig { params(ints: T::Array[Integer]).returns(T::Array[Integer]) }
def takes_ints(ints)
  ints
end

takes_int(returns_nilable)

returns_nilable.abs

takes_ints(T::Array[T.nilable(Integer)].new)
