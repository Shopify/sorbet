# typed: strict

extend T::Sig

#: -> String
def method1; T.unsafe(nil); end
T.reveal_type(method1) # error: Revealed type: `String`

#: -> ::String
def method2; T.unsafe(nil); end
T.reveal_type(method2) # error: Revealed type: `String`
