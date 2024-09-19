# typed: true

module P1; end
module P2; end
module P3; end
module P4; end
module P5; end
module P6; end

##: (?P1, P2) -> void
# def method4(p1 = nil, p2)
#   T.reveal_type(p1) # error: Revealed type: `T.nilable(P1)`
#   T.reveal_type(p2) # error: Revealed type: `P2`
# end

##: (?x: Integer, y: String, ?z: String) -> void
##: (Integer, String) -> void
#def foo(x = 1, y); end

##: (P1, ?P2, *P3) -> void
#def methodX(p1, p2 = nil, *p3); end

# #: (P1, ?P2, *P3, p4: P4, ?p5: P5, **P6) ?{ -> void } -> void
# def methodX(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
#   T.reveal_type(p1) # error: Revealed type: `P1`
#   T.reveal_type(p2) # error: Revealed type: `T.nilable(P2)`
#   T.reveal_type(p3) # error: Revealed type: `T::Array[P3]`
#   T.reveal_type(p4) # error: Revealed type: `P4`
#   T.reveal_type(p5) # error: Revealed type: `T.nilable(P5)`
#   T.reveal_type(p6) # error: Revealed type: `T::Hash[Symbol, P6]`
#   T.reveal_type(block) # error: Revealed type: `T.proc.void`
# end

class Foo
  #: (p: ^() [self: Foo] -> Integer ) ?{ () [self: Foo] -> String } -> void
  def initialize(p: -> { 42 }, &block)
    T.reveal_type(p) # error: Revealed type: `Foo`
    T.reveal_type(block) # error: Revealed type: `T.proc.returns(String)`
    T.reveal_type(p.call) # error: Revealed type: `Integer`
    T.reveal_type(block&.call) # error: Revealed type: `String`
  end
end

Foo.new do |foo|
  T.reveal_type(foo) # error: Revealed type: `Foo`
end
