# typed: true

# Tests nilable ivar narrowing inside a bound block.
# When self is rebound (via sig or T.bind), LoadIvar resolves the declared
# field type from the rebound class on each read. Flow-sensitive narrowing
# does not carry through because each resolution is independent.
#
# To work around this, assign to a local first:
#   name = @maybe
#   if name then name.length end

class Target
  extend T::Sig

  sig {void}
  def initialize
    @maybe = T.let(nil, T.nilable(String))
  end
end

class Host
  extend T::Sig

  sig {params(blk: T.proc.bind(Target).void).void}
  def self.takes_block(&blk); end

  takes_block do
    T.reveal_type(@maybe) # error: type: `T.nilable(String)`

    if @maybe
      T.reveal_type(@maybe) # error: type: `T.nilable(String)`
      @maybe.length # error: Method `length` does not exist on `NilClass` component of `T.nilable(String)`
    end
  end
end
