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

#: (P1, ?P2, *P3, p4: P4, ?p5: P5, **P6) { -> void } -> void
def methodX(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block); end
