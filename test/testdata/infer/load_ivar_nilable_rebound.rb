# typed: true

# Tests nilable ivar narrowing inside a bound block.
# Currently, LoadIvar resolves the declared field type on each read, so
# flow-sensitive narrowing from an `if` guard does not carry through to
# subsequent reads. This is a known limitation — each LoadIvar is an
# independent resolution, unlike Ident which shares a LocalRef.

class Target
  extend T::Sig

  sig {void}
  def initialize
    @maybe = T.let(nil, T.nilable(String))
  end
end

class Host
  def self.takes_block(&blk); end

  takes_block do
    T.bind(self, Target)
    T.reveal_type(@maybe) # error: type: `T.nilable(String)`

    # NOTE: narrowing does not carry through LoadIvar — @maybe remains
    # T.nilable(String) inside the guard. Each LoadIvar resolves the declared
    # field type independently (unlike Ident, which shares a LocalRef that
    # carries narrowing). The existing fallbackLocal propagation at
    # environment.cc:1870 doesn't help because fallbackLocal is an untyped
    # temporary, not the field's local — there's no shared variable for the
    # if-guard narrowing to flow through.
    #
    # To work around this, assign to a local first:
    #   name = @maybe
    #   if name then name.length end
    if @maybe
      T.reveal_type(@maybe) # error: type: `T.nilable(String)`
      @maybe.length # error: Method `length` does not exist on `NilClass` component of `T.nilable(String)`
    end
  end
end
