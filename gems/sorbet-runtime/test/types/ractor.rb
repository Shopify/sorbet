# frozen_string_literal: true
# typed: ignore
require_relative '../test_helper'

class Opus::Types::Test::Ractor < Critic::Unit::UnitTest
  RACTOR_FIXTURE = "#{__dir__}/fixtures/ractor.rb"
  REQUIRES_FINALIZE_FIXTURE = "#{__dir__}/fixtures/ractor_requires_finalize.rb"
  COERCION_FIXTURE = "#{__dir__}/fixtures/ractor_coercion.rb"
  WEAK_CACHE_FIXTURE = "#{__dir__}/fixtures/ractor_weak_cache.rb"

  before do
    unless defined?(::Ractor) && ::Ractor.respond_to?(:shareable_proc)
      skip("Ractor.shareable_proc is required for Ractor support (Ruby 4.0+)")
    end
  end

  it 'allows calling sig-wrapped methods from non-main Ractors after finalize!' do
    result, status = Open3.capture2(RbConfig.ruby, RACTOR_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      finalized: true
      add: 5
      sum: 10
      repeat: ababab
      parallel: [0, 2, 4, 6]
      type_error: Expected type Integer, got type String with value "x"
      main: 30
    EXPECTED
  end

  it 'requires finalize! before sig-wrapped methods can be called from a Ractor' do
    result, status = Open3.capture2(RbConfig.ruby, REQUIRES_FINALIZE_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      without_finalize: RuntimeError
      after_finalize: 5
    EXPECTED
  end

  it 'supports type coercion (T.let/T.cast/generics) from non-main Ractors after finalize!' do
    result, status = Open3.capture2(RbConfig.ruby, COERCION_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      let_valid: 5
      cast: a
      must: 7
      array_build: [1, 2]
      hash_build: {a: 1}
      nilable: nil
      any: "x"
      class_of: Integer
      let_error: Expected type Integer, got type String with value "x"
      ractor_local_cache: true
      main_nilable: nil
      main_array: [1]
    EXPECTED
  end

  it 'T::Private::WeakCache uses a shared map while unfrozen and Ractor-local storage once frozen' do
    result, status = Open3.capture2(RbConfig.ruby, WEAK_CACHE_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      unfrozen_get: true
      unfrozen_shareable: false
      frozen: true
      frozen_shareable: true
      frozen_main_empty: true
      frozen_main_get: true
      ractor_isolated_empty: true
      ractor_local_write: true
      main_unaffected: true
    EXPECTED
  end
end
