# typed: true
# Exercised by test/types/ractor.rb in a subprocess. Defining sigs (or
# subclassing T::Struct) *after* finalize! installs transient stubs/hooks via
# replace_method; those must not be turned into shareable procs (their closures
# capture unshareable state), or definition would raise Ractor::IsolationError.
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

class Early
  extend T::Sig
  sig { params(a: Integer).returns(Integer) }
  def double(a)
    a * 2
  end
end

T::Configuration.finalize!

# A sig defined after finalize! installs a lazy stub via replace_method.
class Late
  extend T::Sig
  sig { params(a: Integer).returns(Integer) }
  def triple(a)
    a * 3
  end
end
puts "late_main: #{Late.new.triple(4)}"

# Once first called (which builds the real validator), it is also Ractor-callable.
puts "late_ractor: #{Ractor.new { Late.new.triple(5) }.value}"

# Subclassing a T::Struct after finalize! exercises the replace_method `inherited` hook.
class Point < T::Struct
  const :x, Integer
end
puts "struct_subclass: #{Point.new(x: 7).x}"
