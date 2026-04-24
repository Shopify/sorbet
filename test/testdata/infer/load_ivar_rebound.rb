# typed: true

# Tests that instance variables resolve correctly inside blocks with a bound self.
# LoadIvar defers ivar resolution to inference time when the ivar doesn't exist
# on the lexical class, allowing it to resolve against the rebound class instead.

class Target
  extend T::Sig

  sig {void}
  def initialize
    @name = T.let("hello", String)
    @count = T.let(42, Integer)
  end
end

class Host
  def self.takes_block(&blk); end

  # Motivating case: ivar read in a block with T.bind resolves against the
  # rebound class. @name and @count don't exist on Host, but do exist on Target.
  takes_block do
    T.bind(self, Target)
    T.reveal_type(self)   # error: type: `Target`
    T.reveal_type(@name)  # error: type: `String`
    T.reveal_type(@count) # error: type: `Integer`
  end

  # Ivar that doesn't exist on either class — stays untyped
  takes_block do
    T.bind(self, Target)
    T.reveal_type(@nonexistent) # error: type: `T.untyped`
  end
end
