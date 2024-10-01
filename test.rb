# typed: strict

extend T::Sig

# #: (String?) -> String
# def foo(x)
#   if ARGV.first
#     puts x
#     return x #:: String
#   end

#   x #:: String
# end

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

x = "hello" #: String?

if ARGV.first
  puts x
  return x #:: String
end
