# frozen_string_literal: true
require_relative '../test_helper'

module Opus::Types::Test
  class LiteralValueTest < Critic::Unit::UnitTest
    describe 'T::Symbol' do
      it 'creates a LiteralValue type' do
        type = T::Symbol(:foo)
        assert_instance_of(T::Types::LiteralValue, type)
      end

      it 'has the correct name' do
        assert_equal('T::Symbol(:foo)', T::Symbol(:foo).name)
      end

      it 'validates matching symbols' do
        type = T::Symbol(:foo)
        assert(type.valid?(:foo))
      end

      it 'rejects non-matching symbols' do
        type = T::Symbol(:foo)
        refute(type.valid?(:bar))
        refute(type.valid?("foo"))
        refute(type.valid?(nil))
      end

      it 'works with T.assert_type!' do
        assert_equal(:foo, T.assert_type!(:foo, T::Symbol(:foo)))
      end

      it 'raises on T.assert_type! mismatch' do
        assert_raises(TypeError) do
          T.assert_type!(:bar, T::Symbol(:foo))
        end
      end

      it 'is a subtype of Symbol' do
        assert(T::Symbol(:foo).subtype_of?(T::Utils.coerce(Symbol)))
      end

      it 'is not a subtype of a different symbol literal' do
        refute(T::Symbol(:foo).subtype_of?(T::Symbol(:bar)))
      end

      it 'is a subtype of the same symbol literal' do
        assert(T::Symbol(:foo).subtype_of?(T::Symbol(:foo)))
      end

      it 'rejects non-symbol arguments' do
        assert_raises(ArgumentError) do
          T::Symbol("foo")
        end
      end
    end

    describe 'T::Utils.coerce with symbols' do
      it 'coerces a bare symbol to LiteralValue' do
        type = T::Utils.coerce(:foo)
        assert_instance_of(T::Types::LiteralValue, type)
        assert_equal(:foo, type.val)
      end

      it 'coerced symbol validates correctly' do
        type = T::Utils.coerce(:foo)
        assert(type.valid?(:foo))
        refute(type.valid?(:bar))
      end
    end
  end
end
