# typed: strict

extend T::Sig

module P1; end
module P2; end
module P3; end
module P4; end
module P5; end
module P6; end

sig { params(p1: P1, p2: T.nilable(P2), p3: P3, p4: P4, p5: T.nilable(P5), p6: P6, block: T.proc.void).void }
def bar(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
end

#: (P1, ?P2, *P3, p4: P4, ?p5: P5, **P6) { -> void } -> void
def foo(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
  T.reveal_type(p1)
  T.reveal_type(p2)
  T.reveal_type(p3)
  T.reveal_type(p4)
  T.reveal_type(p5)
  T.reveal_type(p6)
  T.reveal_type(block)
end
