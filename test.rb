# typed: strict

extend T::Sig

#: (P1, P2 -> void
#
#
def parse_error1; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`
