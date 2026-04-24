# typed: true

# Pins the current write-side behavior for ivars in bound blocks.
# Writes go through the lexical class (Ident/unresolvedIdent2Local), while
# reads go through the rebound class (LoadIvar). This asymmetry is tracked
# for the StoreIvar follow-up.

class Target
  extend T::Sig

  sig {void}
  def initialize
    @name = T.let("hello", String)
  end

  sig {returns(String)}
  def name
    @name
  end
end

class WriteHost
  def self.takes_block(&blk); end

  takes_block do
    T.bind(self, Target)

    # Read resolves against Target → String
    T.reveal_type(@name) # error: type: `String`

    # Write resolves against the lexical class (WriteHost), not Target.
    # Currently no type error because the lexical class has no @name field,
    # so the write creates an undeclared-field-stub.
    @name = 123

    # The read still resolves against Target (String), not the write's
    # undeclared-field-stub. This is the observable asymmetry.
    T.reveal_type(@name) # error: type: `String`
  end
end
