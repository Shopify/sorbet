# frozen_string_literal: true
# typed: ignore
require_relative '../test_helper'

class Opus::Types::Test::WeakCacheTest < Critic::Unit::UnitTest
  before do
    unless defined?(::Ractor) && ::Ractor.respond_to?(:store_if_absent)
      skip("Ractor.store_if_absent is required (Ruby 3.1+)")
    end
  end

  it "stores and retrieves values while unfrozen" do
    cache = T::Private::WeakCache.new(:weak_cache_unit_unfrozen)
    key = Object.new
    value = Object.new
    cache[key] = value
    assert_same(value, cache[key])
  end

  it "is not Ractor-shareable while unfrozen" do
    cache = T::Private::WeakCache.new(:weak_cache_unit_unfrozen_shareable)
    refute(Ractor.shareable?(cache), "an unfrozen WeakCache holds a WeakMap and can't be shareable")
  end

  it "becomes frozen and Ractor-shareable once frozen" do
    cache = T::Private::WeakCache.new(:weak_cache_unit_frozen).freeze
    assert(cache.frozen?)
    assert(Ractor.shareable?(cache), "a frozen WeakCache references only a Symbol and should be shareable")
  end

  it "hands its warm map to the main Ractor's local storage on freeze" do
    cache = T::Private::WeakCache.new(:weak_cache_unit_drop)
    key = Object.new
    value = Object.new
    cache[key] = value
    cache.freeze

    # The entry built before freeze is preserved (handed to the main Ractor's
    # Ractor-local storage), not discarded.
    assert_same(value, cache[key])

    # And the main Ractor can still write/read further entries.
    key2 = Object.new
    value2 = Object.new
    cache[key2] = value2
    assert_same(value2, cache[key2])
  end

  it "isolates storage per Ractor once frozen" do
    cache = T::Private::WeakCache.new(:weak_cache_unit_isolated).freeze
    main_value = Object.new
    cache[Object] = main_value

    empty_before, read_back = Ractor.new(cache) do |c|
      empty = c[Object].nil?
      marker = Object.new
      c[Object] = marker
      [empty, c[Object].equal?(marker)]
    end.value

    assert(empty_before, "a non-main Ractor should start with its own empty storage")
    assert(read_back, "a non-main Ractor should read back its own write")
    assert_same(main_value, cache[Object], "the main Ractor's entry is unaffected by another Ractor")
  end
end
