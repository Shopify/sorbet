# frozen_string_literal: true
# typed: ignore
require_relative '../test_helper'

class Opus::Types::Test::Ractor < Critic::Unit::UnitTest
  RACTOR_FIXTURE = "#{__dir__}/fixtures/ractor.rb"
  REQUIRES_FINALIZE_FIXTURE = "#{__dir__}/fixtures/ractor_requires_finalize.rb"

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
end
