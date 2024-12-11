# typed: strict
# frozen_string_literal: true

class Foo
  #: { (?) -> void } -> void
  def foo(&block); end

  #: (?) -> void
  def bar; end
end
