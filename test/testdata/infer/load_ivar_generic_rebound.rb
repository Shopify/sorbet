# typed: true

# Tests that LoadIvar correctly threads applied type args through
# resultTypeAsSeenFrom. Without this, T.bind(self, Box[Integer]) reading a
# field typed with the class's type member would leak a SelfTypeParam.

class Box
  extend T::Generic
  extend T::Sig

  Elem = type_member

  sig {params(value: Elem).void}
  def initialize(value)
    @value = T.let(value, Elem)
  end
end

class GenericHost
  extend T::Sig

  sig {params(blk: T.proc.void).void}
  def self.takes_block(&blk); end

  takes_block do
    T.bind(self, Box[Integer])
    T.reveal_type(@value) # error: type: `Integer`
  end

  takes_block do
    T.bind(self, Box[String])
    T.reveal_type(@value) # error: type: `String`
  end
end
