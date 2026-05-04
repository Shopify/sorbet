# typed: __STDLIB_INTERNAL

# `T::Boolish` is a stricter alternative to `T::Boolean`. Unlike `T::Boolean`
# (`T.any(TrueClass, FalseClass)`, which exposes every method present on both
# `TrueClass` and `FalseClass`), values typed as `T::Boolish` only support
# truthiness checks (`if b`, `unless b`, `&&`, `||`) and logical negation (`!b`).
#
# `T::Trueish` and `T::Falseish` are synthetic modules with no superclass — in
# particular, they do not inherit from `Object`. The subtype relationships
# `TrueClass <: T::Trueish`, `FalseClass <: T::Falseish`, `NilClass <: T::Falseish`
# are wired directly in `core/types/subtyping.cc` rather than via Ruby inheritance,
# so that `Object`'s methods aren't dispatchable on these types.
#
# Behind `--enable-experimental-boolish-type`.

module T::Trueish
  sig { returns(T::Falseish) }
  def !; end
end

module T::Falseish
  sig { returns(T::Trueish) }
  def !; end
end

T::Boolish = T.type_alias { T.any(T::Trueish, T::Falseish) }
