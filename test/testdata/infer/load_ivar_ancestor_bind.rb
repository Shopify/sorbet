# typed: true

# Tests that binding self to an ancestor class correctly loses access to
# descendant ivars. If Foo declares @x but self is bound to Object (Foo's
# ancestor), @x doesn't exist on Object at runtime, so it should be untyped.

class Foo
  extend T::Sig

  sig {void}
  def initialize
    @x = T.let(42, Integer)
  end
end

class AncestorHost
  extend T::Sig

  sig {params(blk: T.proc.bind(Object).void).void}
  def self.takes_block(&blk); end

  # Self is bound to Object, which doesn't have @x.
  # Even though the lexical class might have it, Object doesn't.
  takes_block do
    T.reveal_type(@x) # error: type: `T.untyped`
  end
end
