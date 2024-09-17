# typed: strict

extend T::Sig

sig { returns({ id: String, name: String }) }
def tuple_type0; T.unsafe(nil); end
T.reveal_type(tuple_type0) # error: Revealed type: `[Integer]`

#: -> { id: String, name: String }
def shape_type1; T.unsafe(nil); end
T.reveal_type(shape_type1) # error: Revealed type: `{ id: String, name: String }`
