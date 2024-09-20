# typed: true

module P1; end
module P2; end
module P3; end
module P4; end
module P5; end
module P6; end

class Abstract
  extend T::Sig
  extend T::Helpers

  abstract!

  # @abstract
  #: -> Integer
  def foo; end
end
