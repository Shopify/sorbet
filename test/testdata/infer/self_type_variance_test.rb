# typed: true
extend T::Sig

class A
  extend T::Sig

  # Public method - variance checking will run
  sig {params(x: T.self_type).returns(T.self_type)}
  def public_identity(x)
    x
  end
end
