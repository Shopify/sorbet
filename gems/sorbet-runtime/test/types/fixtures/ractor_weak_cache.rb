# typed: false
# Exercised by test/types/ractor.rb in a subprocess. Verifies the T::Private::WeakCache
# contract: a shared WeakMap while unfrozen, and Ractor-shareable + Ractor-local
# storage once frozen.
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

KEY = Object.new
VAL = Object.new

cache = T::Private::WeakCache.new(:test_weak_cache)

# While unfrozen: backed by a shared WeakMap, and not Ractor-shareable.
cache[KEY] = VAL
puts "unfrozen_get: #{cache[KEY].equal?(VAL)}"
puts "unfrozen_shareable: #{Ractor.shareable?(cache)}"

cache.freeze
puts "frozen: #{cache.frozen?}"
puts "frozen_shareable: #{Ractor.shareable?(cache)}"

# Once frozen the shared WeakMap is dropped, so storage is Ractor-local: the main
# Ractor starts empty and can write/read its own entry.
puts "frozen_main_empty: #{cache[KEY].nil?}"
cache[KEY] = VAL
puts "frozen_main_get: #{cache[KEY].equal?(VAL)}"

# A non-main Ractor receives the (shareable) cache, sees its own empty storage,
# and writes are isolated to that Ractor.
isolated, wrote = Ractor.new(cache) do |c|
  empty_before = c[Object].nil?
  marker = "ractor-value"
  c[Object] = marker
  [empty_before, c[Object].equal?(marker)]
end.value
puts "ractor_isolated_empty: #{isolated}"
puts "ractor_local_write: #{wrote}"

# The main Ractor's entry is unaffected by the other Ractor.
puts "main_unaffected: #{cache[Object].nil?}"
