# typed: true

module Prism
  class Node; end
  class CallNode < Node; end
  class AssocNode < Node
    attr_reader :key, :value
  end
  class KeywordHashNode < Node
    attr_reader :elements
  end

  class HashNode < Node
    attr_reader :elements
  end
end

class Location; end

class Send < T::Struct
  extend T::Sig

  const :node, Prism::CallNode
  const :name, String
  const :recv, T.nilable(Prism::Node), default: nil
  const :args, T::Array[Prism::Node], default: []
  const :block, T.nilable(Prism::Node), default: nil
  const :location, Location

  # sig do
  #   type_parameters(:T)
  #     .params(arg_type: T::Class[T.type_parameter(:T)], block: T.proc.params(arg: T.type_parameter(:T)).void)
  #     .void
  # end
  #: [TYPE_T] (singleton(TYPE_T) arg_type) { (TYPE_T arg) -> void } -> void
  def each_arg(arg_type, &block)
    args.each do |arg|
      yield(T.unsafe(arg)) if arg.is_a?(arg_type)
    end
  end

  # sig { params(block: T.proc.params(key: Prism::Node, value: T.nilable(Prism::Node)).void).void }
  #: { (Prism::Node key, Prism::Node? value) -> void } -> void
  def each_arg_assoc(&block)
    args.each do |arg|
      next unless arg.is_a?(Prism::KeywordHashNode) || arg.is_a?(Prism::HashNode)

      arg.elements.each do |assoc|
        yield(assoc.key, assoc.value) if assoc.is_a?(Prism::AssocNode)
      end
    end
  end
end
