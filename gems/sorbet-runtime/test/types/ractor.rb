# frozen_string_literal: true
# typed: ignore
require_relative '../test_helper'

class Opus::Types::Test::Ractor < Critic::Unit::UnitTest
  RACTOR_FIXTURE = "#{__dir__}/fixtures/ractor.rb"
  REQUIRES_FINALIZE_FIXTURE = "#{__dir__}/fixtures/ractor_requires_finalize.rb"
  COERCION_FIXTURE = "#{__dir__}/fixtures/ractor_coercion.rb"
  POST_FINALIZE_FIXTURE = "#{__dir__}/fixtures/ractor_post_finalize.rb"
  PROPS_FIXTURE = "#{__dir__}/fixtures/ractor_props.rb"

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
      type_alias: [1, 2, 3]
      let_error: Expected type Integer, got type String with value "x"
      ractor_local_cache: true
      main_nilable: nil
      main_array: [1]
    EXPECTED
  end

  it 'allows defining sigs and subclassing T::Struct after finalize!' do
    result, status = Open3.capture2(RbConfig.ruby, POST_FINALIZE_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      late_main: 12
      late_ractor: 15
      struct_subclass: 7
    EXPECTED
  end

  it 'supports T::Enum and T::Struct (incl. (de)serialization) from non-main Ractors after finalize!' do
    result, status = Open3.capture2(RbConfig.ruby, PROPS_FIXTURE)
    assert(status.success?, "ruby failed (exit #{status.exitstatus})")
    assert_equal(<<~EXPECTED, result)
      enum_serialize: spades
      enum_deserialize: true
      struct_new: 10
      struct_default: hearts
      struct_setter: ["x"]
      serialize: {"rank" => 7, "suit" => "hearts", "tags" => ["a", "b"]}
      deserialize: spades
      type_error: Can't set Card.rank to "nope" (instance of String) - need a Integer
    EXPECTED
  end
end
