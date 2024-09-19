# typed: strict

extend T::Sig

module P1; end
module P2; end
module P3; end
module P4; end
module P5; end
module P6; end

# Sigs

# We do not create any RBI if there is no RBS sigs
def method_no_sig; T.unsafe(nil); end # error: The method `method_no_sig` does not have a `sig`

#: -> String
def method_sig1; T.unsafe(nil); end

#: -> String
def method_sig2; T.unsafe(nil); end

#:      -> String
def method_sig3; T.unsafe(nil); end

# some comment
#: -> String
# some comment
def method_sig4; T.unsafe(nil); end

  #: -> String
# ^^^^^^^^^^^^ error: Unused type annotation. No method def before next annotation
  #: -> void
  def method_sig5; T.unsafe(nil); end

# Parse errors

#: (P1, P2 -> void
#          ^^ error: Failed to parse RBS signature (unexpected token for function parameter name)
def parse_error1; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`

class ParseError2
  #: void
  #  ^^^^ error: Failed to parse RBS signature (expected a token `pARROW`)
  def parse_error2(p1, p2); end # error: The method `parse_error2` does not have a `sig`

  class ParseError3
    # Some comment
    #: v
    #  ^ error: Failed to parse RBS signature (expected a token `pARROW`)
    # Some comment
    def parse_error3(p1, p2); end # error: The method `parse_error3` does not have a `sig`
  end
end

  #:
 #  ^ error: Failed to parse RBS signature (expected a token `pARROW`)
  def parse_error4(p1, p2); end # error: The method `parse_error4` does not have a `sig`

# # Args

#: (P1) -> void
def method1(p1)
  T.reveal_type(p1) # error: Revealed type: `P1`
end

#: (P1, P2) -> void
def method2(p1, p2)
  T.reveal_type(p1) # error: Revealed type: `P1`
  T.reveal_type(p2) # error: Revealed type: `P2`
end

#: (?P1) -> void
def method3(p1 = nil)
  T.reveal_type(p1) # error: Revealed type: `T.nilable(P1)`
end

#: (?P1, P2) -> void
def method4(p1 = nil, p2)
  T.reveal_type(p1) # error: Revealed type: `T.nilable(P1)`
  T.reveal_type(p2) # error: Revealed type: `P2`
end

# Named args

#: (String x) -> String
def named_args1(x)
  T.reveal_type(x) # error: Revealed type: `String`
end

#: (String x) -> String
#   ^^^^^^^^ error: Unknown argument name `x`
def named_args2(y)
  #             ^ error: Malformed `sig`. Type not specified for argument `y`
  T.reveal_type(y) # error: Revealed type: `T.untyped`
end

#: (String foo) -> String
#   ^^^^^^^^^^ error: Unknown argument name `foo`
def named_args3(y)
  #             ^ error: Malformed `sig`. Type not specified for argument `y`
  T.reveal_type(y) # error: Revealed type: `T.untyped`
end

#: (String ?x) -> void
def named_args4(x)
  T.reveal_type(x) # error: Revealed type: `T.nilable(String)`
end

#: (?String x) -> void
def named_args5(x)
  T.reveal_type(x) # error: Revealed type: `T.nilable(String)`
end

#: (String x) -> void
def named_args5(x = nil) # error: Argument does not have asserted type `String`
  T.reveal_type(x) # error: Revealed type: `T.nilable(String)`
end

#: (String ?x) -> void
def named_args6(x = nil)
  T.reveal_type(x) # error: Revealed type: `T.nilable(String)`
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
