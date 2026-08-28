# typed: strict
# enable-experimental-rbs-comments: true

# @abstract
class Abstract
  # @abstract
  #: -> void
  def foo; end # error: Methods declared @abstract with an RBS comment must always call super

  # @abstract
  #: -> void
  def bar # error: Methods declared @abstract with an RBS comment must always call super
  end

  # @abstract
  #: -> void
  def baz # error: Methods declared @abstract with an RBS comment must always call super
    puts # error: Abstract methods must not contain any code in their body
  end

  # @abstract
  #: -> void
  def qux # error: Methods declared @abstract with an RBS comment must always call super
    puts # error: Abstract methods must not contain any code in their body
    puts
  end

  # @abstract
  #: -> void
  def self.foo; end # error: Methods declared @abstract with an RBS comment must always call super

  # @abstract
  #: -> void
  def self.bar # error: Methods declared @abstract with an RBS comment must always call super
  end

  # @abstract
  #: -> void
  def self.baz # error: Methods declared @abstract with an RBS comment must always call super
    puts # error: Abstract methods must not contain any code in their body
  end

  # @abstract
  #: -> void
  def self.qux # error: Methods declared @abstract with an RBS comment must always call super
    puts # error: Abstract methods must not contain any code in their body
    puts
  end
end
