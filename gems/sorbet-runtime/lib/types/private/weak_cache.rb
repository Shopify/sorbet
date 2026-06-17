# frozen_string_literal: true
# typed: false

module T::Private
  # A cache keyed by weak references, backed by an `ObjectSpace::WeakMap`.
  #
  # While unfrozen it uses a single process-shared WeakMap -- the fast path for
  # normal, single-Ractor execution.
  #
  # `freeze` nils out the shared WeakMap so the cache itself references only a
  # Symbol and is therefore Ractor-shareable (a frozen object holding a WeakMap
  # would not be, and so couldn't be read from a module variable by a non-main
  # Ractor). A frozen cache serves each Ractor from its own Ractor-local
  # WeakMap, so it can be read and written from non-main Ractors, which cannot
  # touch the un-shareable shared WeakMap.
  #
  # `T::Configuration.finalize!` freezes every WeakCache, which is what lets the
  # type-coercion pools work from any Ractor.
  class WeakCache
    # Freeze every WeakCache, switching them all to Ractor-local storage. Called
    # from `finalize!` in the main Ractor. `ObjectSpace.each_object` is slow, but
    # `finalize!` runs once, and this avoids a registry / per-cache registration
    # on the common (non-finalized) path.
    def self.freeze_all!
      ObjectSpace.each_object(self) { |cache| cache.freeze }
      nil
    end

    def initialize(ractor_local_key)
      @ractor_local_key = ractor_local_key
      @weak_map = ObjectSpace::WeakMap.new
    end

    def [](key)
      weak_map[key]
    end

    def []=(key, value)
      weak_map[key] = value
    end

    # Drop the shared WeakMap so the frozen cache holds only a Symbol and is
    # therefore Ractor-shareable. Frozen caches use Ractor-local storage.
    #
    # Hand the already-built map to this (main) Ractor's local storage rather than
    # discarding it, so the warm cache survives `finalize!` (and the main Ractor
    # keeps returning the same coerced type instances it built before). Other
    # Ractors still build their own.
    def freeze
      unless frozen?
        built = @weak_map
        Ractor.store_if_absent(@ractor_local_key) { built }
        @weak_map = nil
      end
      super
    end

    private

    def weak_map
      @weak_map || Ractor.store_if_absent(@ractor_local_key) { ObjectSpace::WeakMap.new }
    end
  end
end
