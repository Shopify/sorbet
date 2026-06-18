# frozen_string_literal: true
# typed: true

module T::Private::Types
  # Wraps a proc for a type alias to defer its evaluation.
  class TypeAlias < T::Types::Base
    # Make every type alias Ractor-shareable, so the constants holding them can be
    # read from non-main Ractors. Called by `T::Configuration.finalize!`.
    #
    # `ObjectSpace.each_object` is slow, but `finalize!` runs once, and this avoids
    # having every alias register itself on the common (non-finalized) path.
    def self.finalize!
      ObjectSpace.each_object(self) { |type_alias| T::Private::Methods.make_shareable(type_alias) }
    end

    def initialize(callable)
      @callable = callable
    end

    def build_type
      nil
    end

    def aliased_type
      @aliased_type ||= T::Utils.coerce(@callable.call)
    end

    # overrides Object#freeze
    #
    # Resolve the deferred alias and drop the (un-shareable) `@callable` block
    # before freezing, so the frozen alias references only its resolved type and
    # can be made Ractor-shareable. The resolved type realizes itself in its own
    # `freeze` (see TypedEnumerable#freeze) as `make_shareable` deep-freezes it.
    def freeze
      unless frozen?
        aliased_type
        @callable = nil
      end
      super
    end

    # overrides Base
    def name
      aliased_type.name
    end

    # overrides Base
    def recursively_valid?(obj)
      aliased_type.recursively_valid?(obj)
    end

    # overrides Base
    def valid?(obj)
      aliased_type.valid?(obj)
    end
  end
end
