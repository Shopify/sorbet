# typed: strict
# frozen_string_literal: true

class Final
  extend T::Helpers

  final!

  # @final
  #: -> void
  def foo; end
end
