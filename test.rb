# typed: strict

extend T::Sig

#: (String?) -> String
def foo(x)
  if ARGV.first
    puts x
    return x #:: String
  end

  x #:: String
end

# if ARGV.first
#   z1 = ARGV.first #:: String
#   T.reveal_type(z1)

#   ARGV.each do |arg|
#     z2 = ARGV.first #:: String
#     T.reveal_type(z2)
#   end
# end

# y = nil #: Integer?
# T.reveal_type(y)

# x = "hello" #: String?

# if ARGV.first
#   puts x
#   return x #:: String
# end


# class TypePrinter
#   extend T::Sig

#   #: String
#   attr_reader :string

#   #: -> void
#   def initialize
#     @string = String.new #: String
#   end
# end

# x = 42 #:: untyped

# #: (Array[String?]) -> Array[String]
# def foo(x)
#   x.sort do |y|
#     a = x[0] #:: String
#     T.reveal_type(a)
#     b = x[1] #:: String
#     T.reveal_type(b)
#     res = a <=> b
#     T.reveal_type(res)
#     res
#   end
#   x.compact
# end

# x = ARGV.first
# case x
# when "foo"
#   x #:: String
# else
#   x #:: Integer
# end

# x = [1, 2, 3].to_h { |x| [x, "#{x}"] } #: Hash[Integer, String]

# @abstract
# class C
# end

# @interface
# module I
# end
