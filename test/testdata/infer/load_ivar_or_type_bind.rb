# typed: true

# Tests the OrType/AndType fallback path in LoadIvar. When self is bound to a
# union type, resolveIvarOnSelfType can't extract a single class and falls back
# to untyped. The loadIvar.unhandledSelfType counter fires for telemetry.

class Cat
  extend T::Sig

  sig {void}
  def initialize
    @name = T.let("Whiskers", String)
  end
end

class Dog
  extend T::Sig

  sig {void}
  def initialize
    @name = T.let("Rex", String)
  end
end

class OrTypeHost
  extend T::Sig

  sig {params(blk: T.proc.void).void}
  def self.takes_block(&blk); end

  # T.any(Cat, Dog) produces an OrType for self — LoadIvar can't resolve a
  # single class, so @name falls back to untyped.
  takes_block do
    T.bind(self, T.any(Cat, Dog))
    T.reveal_type(@name) # error: type: `T.untyped`
  end
end
