# typed: true
# Exercised by test/types/ractor.rb in a subprocess. Covers type coercion
# (T.let/T.cast/T.must and generic type builders) from non-main Ractors after
# finalize!, plus a main-Ractor regression guard for memoization on the pooled
# type objects that finalize! freezes.
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

class Box
  extend T::Sig

  # Reference Integer/String in a sig so their pooled Simple types are frozen by
  # finalize! -- this is what would otherwise break `to_nilable` memoization.
  sig { params(n: Integer, s: String).returns(Integer) }
  def measure(n, s)
    n + s.length
  end
end

T::Configuration.finalize!

# Coercion and cast operations from non-main Ractors.
puts "let_valid: #{Ractor.new { T.let(5, Integer) }.value}"
puts "cast: #{Ractor.new { T.cast('a', String) }.value}"
puts "must: #{Ractor.new { T.must(7) }.value}"
puts "array_build: #{Ractor.new { T.let([1, 2], T::Array[Integer]) }.value.inspect}"
puts "hash_build: #{Ractor.new { T.let({a: 1}, T::Hash[Symbol, Integer]) }.value.inspect}"
puts "nilable: #{Ractor.new { T.let(nil, T.nilable(Integer)) }.value.inspect}"
puts "any: #{Ractor.new { T.let('x', T.any(String, Integer)) }.value.inspect}"
puts "class_of: #{Ractor.new { T.let(Integer, T.class_of(Integer)) }.value}"

# A failed T.let inside a Ractor surfaces the expected TypeError, which exercises
# the coercion pool on the error path.
let_error = Ractor.new do
  T.let('x', Integer)
  'no error raised'
rescue TypeError => e
  /Expected.*got type.*$/.match(e.message)&.to_s
end.value
puts "let_error: #{let_error}"

# Repeated coercion within a Ractor reuses the Ractor-local cache without raising.
puts "ractor_local_cache: #{Ractor.new { T.let([1], T::Array[Integer]); T.let([2], T::Array[Integer]); true }.value}"

# The main Ractor still works after finalize! for operations that memoize on the
# now-frozen pooled type objects (regression guard for `to_nilable`).
puts "main_nilable: #{T.let(nil, T.nilable(Integer)).inspect}"
puts "main_array: #{T.let([1], T::Array[Integer]).inspect}"
