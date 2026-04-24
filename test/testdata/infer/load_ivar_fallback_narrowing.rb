# typed: true

# Tests that LoadIvar's Tier-3 fallback (no rebind) preserves flow-sensitive
# narrowing. When self is not rebound, LoadIvar returns the fallbackLocal —
# the same LocalRef that Ident would use — so narrowing flows through.

class MyClass
  extend T::Sig

  sig {void}
  def initialize
    @value = T.let(0, Integer)
    @nilable = T.let(nil, T.nilable(String))
  end

  sig {void}
  def test_ivar_in_block
    # @value resolves lexically on MyClass → LoadIvar falls through to fallbackLocal
    [1, 2, 3].each do |x|
      T.reveal_type(@value) # error: type: `Integer`
    end
  end

  sig {void}
  def test_narrowing_preserved
    # Flow-sensitive narrowing must work through LoadIvar in non-rebound blocks.
    if @nilable
      T.reveal_type(@nilable) # error: type: `String`

      [1].each do
        # Inside the block, @nilable is still narrowed because LoadIvar
        # returns the fallbackLocal when self is not rebound.
        T.reveal_type(@nilable) # error: type: `String`

        # Call a String method to prove the narrowed type is actually usable.
        @nilable.length
      end
    end
  end
end
