# typed: false

-> { 123 }
-> (foo) { foo }
-> (foo = 123) { foo }
-> { 123 }.call
lambda { 123 }
lambda { |foo| foo }

foo :arg1, -> { 123 }

# Even with a class, the receiver is still Kernel
class C
  def foo
    -> { 123 }
  end
end
