# typed: strict
# frozen_string_literal: true

class Final
  # @final
  #: ?{ (?) -> untyped } -> void
  def foo(&block); end
end
