# typed: true

class AbstractClassVisibility
  extend T::Sig
  extend T::Helpers
  abstract!

  sig {abstract.returns(Object)}
  def foo; end

  sig {abstract.returns(Object)}
  def bar; end

  sig {abstract.returns(Object)}
  private def baz; end

  sig {abstract.returns(Object)}
  protected def qux; end

  sig {abstract.returns(Object)}
  protected def quux; end

  sig {abstract.returns(Object)}
  private def quuz; end

  sig {abstract.returns(Object)}
  protected def corge; end
end

class ImplVisibility < AbstractClassVisibility
  sig {override.returns(Object)}
  protected def foo; end
          # ^^^^^^^ error: Method `foo` is protected in `ImplVisibility` but not in `AbstractClassVisibility`
  private def bar; end
        # ^^^^^^^ error: Method `bar` is private in `ImplVisibility` but not in `AbstractClassVisibility`

  sig {override.returns(Object)}
  protected def baz; end

  sig {override.returns(Object)}
  private def qux; end
        # ^^^^^^^ error: Method `qux` is private in `ImplVisibility` but not in `AbstractClassVisibility`

  sig {override.returns(Object)}
  protected def quux; end

  sig {override.returns(Object)}
  private def quuz; end

  sig {override.returns(Object)}
  protected def corge; end
end
