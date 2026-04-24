# typed: true

# Tests that sig-based rebind (Tier-1) takes precedence over body-based
# T.bind(self, X) (Tier-2) for ivar resolution. This mirrors LoadSelf's
# behavior where the sig's bind type determines self.

class A
  extend T::Sig

  sig {void}
  def initialize
    @x = T.let(1, Integer)
  end
end

class B
  extend T::Sig

  sig {void}
  def initialize
    @x = T.let("hello", String)
  end
end

class ShadowHost
  extend T::Sig

  sig {params(blk: T.proc.bind(A).void).void}
  def self.takes_block(&blk); end

  # Sig says bind to A, body says bind to B.
  # Ivar resolution follows the sig (Tier-1), so @x resolves as Integer (from A).
  takes_block do
    T.bind(self, B)
    T.reveal_type(self) # error: type: `B`
    T.reveal_type(@x)   # error: type: `Integer`
  end
end
