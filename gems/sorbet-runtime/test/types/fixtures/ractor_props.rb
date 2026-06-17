# typed: true
# Exercised by test/types/ractor.rb in a subprocess. After finalize!, T::Enum
# values, T::Struct instances, and their (de)serialization can be used from
# non-main Ractors (finalize! makes each prop decorator and its type graph
# Ractor-shareable).
Warning[:experimental] = false
require_relative '../../../lib/sorbet-runtime'

class Suit < T::Enum
  enums do
    Hearts = new
    Spades = new
  end
end

class Card < T::Struct
  const :rank, Integer
  prop :suit, Suit, default: Suit::Hearts
  prop :tags, T::Array[String], default: []
end

T::Configuration.finalize!

# T::Enum from a non-main Ractor.
puts "enum_serialize: #{Ractor.new { Suit::Spades.serialize }.value}"
puts "enum_deserialize: #{Ractor.new { Suit.from_serialized('hearts') == Suit::Hearts }.value}"

# T::Struct construction, accessors and defaults from a non-main Ractor.
puts "struct_new: #{Ractor.new { Card.new(rank: 10).rank }.value}"
puts "struct_default: #{Ractor.new { Card.new(rank: 1).suit.serialize }.value}"
puts "struct_setter: #{Ractor.new { c = Card.new(rank: 1); c.tags = ['x']; c.tags }.value.inspect}"

# Serialize / deserialize from a non-main Ractor (generated methods, generic prop types).
puts "serialize: #{Ractor.new { Card.new(rank: 7, tags: ['a', 'b']).serialize }.value.inspect}"
puts "deserialize: #{Ractor.new { Card.from_hash({ 'rank' => 3, 'suit' => 'spades', 'tags' => ['z'] }).suit.serialize }.value}"

# A prop type violation inside a Ractor surfaces the expected TypeError.
type_error = Ractor.new do
  Card.new(rank: 'nope')
  'no error raised'
rescue TypeError => e
  /Can't set Card\.rank.*?need a Integer/.match(e.message)&.to_s
end.value
puts "type_error: #{type_error}"
