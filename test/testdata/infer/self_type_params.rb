# typed: true
extend T::Sig

class A
  extend T::Sig

  sig {params(x: T.self_type).returns(T.self_type)}
  def identity(x)
    T.reveal_type(x) # error: Revealed type: `A`
    x
  end

  sig {params(other: T.self_type).returns(T::Boolean)}
  def same_as?(other)
    T.reveal_type(other) # error: Revealed type: `A`
    self.object_id == other.object_id
  end
end

class B < A
end

a = A.new
b = B.new

T.reveal_type(a.identity(a)) # error: Revealed type: `A`
a.identity(b) # OK: B is a subclass of A

T.reveal_type(b.identity(b)) # error: Revealed type: `B`
b.identity(a) # error: Expected `B` but found `A`
