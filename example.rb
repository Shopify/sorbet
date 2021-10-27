# typed: true

class Person
  extend T::Sig

  sig { params(block: T.proc.bind(T.attached_class).void).void }
  def self.foo(&block)
  end
end

Person.foo do
  T.reveal_type(self)
end
