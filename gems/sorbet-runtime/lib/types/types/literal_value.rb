# frozen_string_literal: true
# typed: true

module T::Types
  class LiteralValue < Base
    attr_reader :val

    def initialize(val)
      raise ArgumentError.new("LiteralValue expects a Symbol, got #{val.class}") unless val.is_a?(::Symbol)
      @val = val
    end

    def build_type
      nil
    end

    # overrides Base
    def name
      @name ||= "T::Symbol(:#{val})".freeze
    end

    # overrides Base
    def valid?(obj)
      obj == @val
    end

    # overrides Base
    private def subtype_of_single?(other)
      case other
      when LiteralValue
        @val == other.val
      when Simple
        @val.is_a?(other.raw_type)
      else
        false
      end
    end
  end
end
