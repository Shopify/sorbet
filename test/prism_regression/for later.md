Handle variable binding in Array patterns


case foo
in [head, *tail]
  puts "An array-like thing that starts with #{head} and ends with #{tail}"
in [*init, last]
  puts "An array-like thing that starts with #{init} and ends with #{last}"
in { k: value }
  puts "A hash-like whose key `:k` has value #{value}"
in [[value], *tail] # Array pattern inside an Array pattern
  puts "An array-like thing that starts with a one-element Array containing #{value}, and ends with #{tail}"
in { k: [value] } # Array pattern inside a Hash pattern
  puts "A hash-like whose key `:k` has a one-element Array value containing #{value}"
in [{ k: value }, *tail] # A Hash pattern inside an Array pattern
  puts "An array-like thing that starts with a one-element Hash containing #{value}, and ends with #{tail}"
in { k: { k2: value } } # A Hash pattern inside a Hash pattern
  puts "A hash-like whose key `:k` has a one-element Hash value containing k2: #{value}"
in x # PM_LOCAL_VARIABLE_TARGET_NODE => MatchVar
  puts "Some other value: #{x}"
end
