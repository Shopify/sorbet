# typed: strict

extend T::Sig

module A; end
module B; end
module C; end

#: -> void
def type; T.unsafe(nil); end
T.reveal_type(type)
