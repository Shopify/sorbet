# typed: false

case foo
in 1
  puts "one!"
in 2
  puts "two!"
in 3 | 4
  puts "three or four!"
else
  puts "Who knows!"
end

case array_like_thing
in []
  puts "empty!"
in [1, 2]
  puts "one and two!"
in 3, 4 # An array pattern without [], but otherwise similar to the one above
  puts "three and four!"
in [5, *]
  puts "starts with five!"
in [*, 6]
  puts "ends with six!"
in [*, 7, *] # A "find pattern"
  puts "contains a seven!"
end

case hash_like_thing
in {}
  puts "empty!"
in { a: 1, b: 2 }
  puts "contains a and b, and maybe other stuff!"
in { c: 3, ** }
  puts "has c, and maybe other stuff!"
in { d: 4, **nil }
  puts "has d and nothing else!"
end

# no else
case foo
in 1
  "one!"
  puts "surprise, multi-line!"
end

case bar
in x if x == 1
  "in with if"
in a, b if b == 2
  "in with 2 args and if"
in c, d; c if c == 3
  "in with 2 args, semicolon, and if"
end
