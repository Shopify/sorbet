# typed: false

-> { 123 }
-> (foo) { foo }
-> (foo = 123) { foo }
-> { 123 }.call
lambda { 123 }
lambda { |foo| foo }

# Test that lambda arguments are translated correctly
foo :arg1, -> { 123 }

# Test lambda literals with numbered arguments
-> { _1 + _2 }

# Even with a class, the receiver is still Kernel
class C
  def foo
    -> { 123 }
  end
end
