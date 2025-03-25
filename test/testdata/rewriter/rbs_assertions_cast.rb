# typed: strict
# enable-experimental-rbs-signatures: true
# enable-experimental-rbs-assertions: true

# errors

err1 = ARGV.first #: as
                  #  ^^ error: Failed to parse RBS type (unexpected token for simple type)

err2 = ARGV.first #: as -> String
                  #     ^^ error: Failed to parse RBS type (unexpected token for simple type)

# assigns

as1 = ARGV.first #:as String
T.reveal_type(as1) # error: Revealed type: `String`

as2 = ARGV.first #: as String
T.reveal_type(as2) # error: Revealed type: `String`

as3 = ARGV.first #:      as      String
T.reveal_type(as3) # error: Revealed type: `String`

as4 = ARGV.first #: as String#some comment
T.reveal_type(as4) # error: Revealed type: `String`

as5 = ARGV.first #: as String # some comment
T.reveal_type(as5) # error: Revealed type: `String`

# classes

class ClassErr1 #: as String # error: Unexpected RBS assertion comment found after `class` declaration
end

class ClassErr2
end #: as String # error: Unexpected RBS assertion comment found after `class` end

# TODO:
# class ClassErr3
#   #: as String
# end

module ModuleErr1 #: as String # error: Unexpected RBS assertion comment found after `module` declaration
end

module ModuleErr2
end #: as String # error: Unexpected RBS assertion comment found after `module` end

# TODO:
# module ModuleErr3
#   #: as String
# end

# methods

#: (String?) -> String
def def1(x)
  x #: as String
end

#: (String?) -> String
def def2(x)
  puts x
  x #: as String
end

#: (String?) -> String
def def3(x)
  return x #: as String
end

#: (String?) -> void
def def4(x)
  return #: as String # error: Unexpected RBS assertion comment found in `return` without an expression
end

#: -> void
def def5
end #: as String # error: Unexpected RBS assertion comment found after `method` end

#: -> void
def def6; end #: as String # error: Unexpected RBS assertion comment found after `method` end

#: -> void
def def7 #: as String # error: Unexpected RBS assertion comment found after `method` declaration
end

#: -> void
def self.def6
end #: as String # error: Unexpected RBS assertion comment found after `method` end

#: -> void
def self.def7; end #: as String # error: Unexpected RBS assertion comment found after `method` end

#: -> void
def self.def8 #: as String # error: Unexpected RBS assertion comment found after `method` declaration
end

# blocks

#: { (String?) -> String } -> void
def take_block(&blk); end

take_block do |x|
  x #: as String
end

take_block do |x|
  puts x
  x #: as String
end

# ifs

#: (untyped) -> void
def if1(x)
  y = if x
    x #: as String
  end
  T.reveal_type(y) # error: Revealed type: `T.nilable(String)`
end

#: (untyped) -> void
def if2(x)
  y = if x
  else
    x #: as String
  end
  T.reveal_type(y) # error: Revealed type: `T.nilable(String)`
end

#: (untyped) -> void
def if2(x)
  y = if x
    x #: as Integer
  else
    x #: as String
  end
  T.reveal_type(y) # error: Revealed type: `T.any(Integer, String)`
end

# breaks

#: (Array[String?]) -> (Array[String?] | String)
def break1(arr)
  arr.each do |x|
    break x #: as String
  end
end

#: (Array[String?]) -> Array[String?]?
def break2(arr)
  arr.each do |x|
    break #: as String # error: Unexpected RBS assertion comment found in `break` without an expression
  end
end

# next

#: (Array[String?]) -> Array[String]
def next1(arr)
  arr.collect do |x|
    next x #: as String
  end
end

#: (Array[String?]) -> Array[String?]
def next2(arr)
  arr.collect do |x|
    next #: as String # error: Unexpected RBS assertion comment found in `next` without an expression
  end
end

# rescue

#: (String?) -> String
def rescue1(x)
  x rescue x #: as String
end

#: (String?) -> String
def rescue2(x)
  x #: as String
rescue => e
  e #: as String
end

#: (String?) -> String
def rescue3(x)
  x #: as String
rescue => e
  puts e
  e #: as String
end

#: (String?) -> String
def rescue4(x)
  puts x
  x #: as String
rescue => e
  puts e
  e #: as String
end

# inseqs

# begin #: as String # error Unexpected RBS assertion comment found after `begin` declaration
#   puts "begin1"
#   puts "begin1"
# end

# begin
#   puts "begin2"
#   puts "begin2"
# end #: as String # error Unexpected RBS assertion comment found after `begin` end

# begin; puts "begin3"; puts "begin3"; end #: as String # error Unexpected RBS assertion comment found after `begin` end
