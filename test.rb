# typed: strict

extend T::Sig

sig { params(p1: Integer, p2: T.nilable(String), p3: String, p4: Integer, p5: T.nilable(String), p6: Integer, block: T.proc.void).void }
def bar(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
end

#: (Integer, ?String, *String, p4: Integer, ?p5: String, **Integer) { -> void } -> void
def foo(p1, p2 = nil, *p3, p4:, p5: nil, **p6, &block)
end
