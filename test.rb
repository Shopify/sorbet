# typed: true

class Parent; end

x1 = Class.new
T.reveal_type(x1) # error: Revealed type: `Class`

x2 = Class.new(Parent)
T.reveal_type(x2) # error: Revealed type: `Parent`

x3 = Class.new(Foo)
T.reveal_type(x3)
