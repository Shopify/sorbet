# typed: true
extend T::Sig

class A
  extend T::Sig

  sig do
    type_parameters(:U)
      .params(
        blk: T.proc.params(x: T.self_type).returns(T.type_parameter(:U)),
      )
      .returns(T.type_parameter(:U))
  end
  def proc_with_self_type_param(&blk)
    blk.call(self)
  end
end

a = A.new
res = a.proc_with_self_type_param do |x|
  T.reveal_type(x) # error: Revealed type: `A`
  1
end
T.reveal_type(res) # error: Revealed type: `Integer`
