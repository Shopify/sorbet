# typed: true
# Exercised by test/types/ractor.rb in a subprocess. Documents that, without
# `T::Configuration.finalize!`, a sig-wrapped method cannot be called from a
# non-main Ractor: its validator is defined with an un-shareable Proc.
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

class Calc
  extend T::Sig

  sig { params(a: Integer, b: Integer).returns(Integer) }
  def add(a, b)
    a + b
  end
end

# No finalize! call here on purpose.
result = Ractor.new do
  Calc.new.add(2, 3)
  'no error raised'
rescue => e
  e.class.name
end.value
puts "without_finalize: #{result}"

# After finalize!, the same call succeeds.
T::Configuration.finalize!
puts "after_finalize: #{Ractor.new { Calc.new.add(2, 3) }.value}"
