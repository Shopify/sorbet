# typed: true

class Delegatee
  def delegated_method; end
end

class Direct
  extend T::Sig
  extend T::Helpers

  def method_missing(name)
    Delegatee.new.send(name)
  end

  delegates_missing_methods_to { Bar }
end

Direct.new.delegated_method


class DelegatingModule
  extend T::Sig
  extend T::Helpers

  def method_missing(name)
    Delegatee.new.send(name)
  end

  delegates_missing_methods_to { Bar }
end

class ViaModule
  include DelegatingModule
end

ViaModule.new.delegated_method
