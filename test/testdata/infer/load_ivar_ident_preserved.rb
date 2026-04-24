# typed: true

# Negative test: when an ivar resolves against the lexical class, the normal
# Ident path is used (not LoadIvar). This preserves flow-sensitive narrowing
# and avoids any overhead.

class MyClass
  extend T::Sig

  sig {void}
  def initialize
    @value = T.let(0, Integer)
    @nilable = T.let(nil, T.nilable(String))
  end

  sig {void}
  def test_ivar_in_block
    # @value resolves lexically on MyClass → Ident path, not LoadIvar
    [1, 2, 3].each do |x|
      T.reveal_type(@value) # error: type: `Integer`
    end
  end

  sig {void}
  def test_narrowing_preserved
    # Flow-sensitive narrowing must work — this is the key regression test.
    # If LoadIvar were emitted for resolved ivars, narrowing would be lost.
    if @nilable
      T.reveal_type(@nilable) # error: type: `String`

      [1].each do
        # Inside the block, @nilable is still narrowed because it resolved
        # lexically and uses Ident (not LoadIvar).
        T.reveal_type(@nilable) # error: type: `String`

        # Call a String method to prove the narrowed type is actually usable.
        # If narrowing were lost, this would error with "method does not exist
        # on NilClass component of T.nilable(String)".
        @nilable.length
      end
    end
  end
end
