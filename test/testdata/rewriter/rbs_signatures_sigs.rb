# typed: strict

extend T::Sig

module P1; end
module P2; end
module P3; end
module P4; end
module P5; end
module P6; end

# Method sigs

#: (P1, P2) -> void
def method1(p1, p2)
  T.reveal_type(p1) # error: Revealed type: `P1`
  T.reveal_type(p2) # error: Revealed type: `P2`
end

#: (?P1) -> void
def method2(p1 = nil)
  T.reveal_type(p1) # error: Revealed type: `T.nilable(P1)`
end

#: (P1, ?P2, *P3, p4: P4, ?p5: P5, **P6) { -> void } -> void
def methodX(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
  T.reveal_type(p1) # error: Revealed type: `P1`
  T.reveal_type(p2) # error: Revealed type: `T.nilable(P2)`
  T.reveal_type(p3) # error: Revealed type: `T::Array[P3]`
  T.reveal_type(p4) # error: Revealed type: `P4`
  T.reveal_type(p5) # error: Revealed type: `T.nilable(P5)`
  T.reveal_type(p6) # error: Revealed type: `T::Hash[Symbol, P6]`
  T.reveal_type(block) # error: Revealed type: `T.proc.void`
end
