# typed: true
# Exercised by test/types/ractor.rb in a subprocess (finalize! is a one-way,
# process-wide operation, so it cannot run inside the shared test process).
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

class Calc
  extend T::Sig

  sig { params(a: Integer, b: Integer).returns(Integer) }
  def add(a, b)
    a + b
  end

  sig { params(xs: T::Array[Integer]).returns(Integer) }
  def sum(xs)
    xs.sum
  end

  sig { params(s: String, n: Integer).returns(String) }
  def self.repeat(s, n)
    s * n
  end
end

# Prepare the runtime in the main Ractor, after all typed classes are loaded.
count = T::Configuration.finalize!
puts "finalized: #{count > 0}"

# A valid instance-method call from a non-main Ractor.
puts "add: #{Ractor.new { Calc.new.add(2, 3) }.value}"

# A valid collection-typed call from a non-main Ractor.
puts "sum: #{Ractor.new { Calc.new.sum([1, 2, 3, 4]) }.value}"

# A valid singleton-method call from a non-main Ractor.
puts "repeat: #{Ractor.new { Calc.repeat('ab', 3) }.value}"

# Many Ractors calling typed methods in parallel.
ractors = 4.times.map { |i| Ractor.new(i) { |n| Calc.new.add(n, n) } }
puts "parallel: #{ractors.map(&:value).inspect}"

# A type violation inside a Ractor surfaces the expected TypeError.
message = Ractor.new do
  Calc.new.add('x', 1)
  'no error raised'
rescue TypeError => e
  /Expected.*got type.*$/.match(e.message)&.to_s
end.value
puts "type_error: #{message}"

# The main Ractor still works after finalize!.
puts "main: #{Calc.new.add(10, 20)}"
