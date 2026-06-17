# frozen_string_literal: true
# typed: true

module T::Types
  # validates that the provided value is within a given set/enum
  class Enum < Base
    extend T::Sig

    attr_reader :values

    def initialize(values)
      case values
      when Hash
        @values = values
      else
        require "set" unless defined?(Set)
        @values = values.to_set
      end
    end

    def build_type
      nil
    end

    # overrides Base
    def valid?(obj)
      @values.member?(obj)
    end

    # overrides Base
    private def subtype_of_single?(other)
      case other
      when Enum
        (@values - other.values).empty?
      else
        false
      end
    end

    # overrides Base
    def name
      @name ||= "T.deprecated_enum([#{@values.map(&:inspect).sort.join(', ')}])"
    end

    # overrides Object#freeze
    #
    # Memoize `name` before freezing so it can still be read afterward (the
    # `@name ||=` above can't write to a frozen object), e.g. when building a
    # type-error message from a Ractor after `finalize!`.
    def freeze
      name unless frozen?
      super
    end

    # overrides Base
    def describe_obj(obj)
      obj.inspect
    end
  end
end
