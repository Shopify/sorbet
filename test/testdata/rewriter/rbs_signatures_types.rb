# typed: strict

extend T::Sig

#: -> String
def method1; T.unsafe(nil); end
T.reveal_type(method1) # error: Revealed type: `String`

#: -> ::String
def method2; T.unsafe(nil); end
T.reveal_type(method2) # error: Revealed type: `String`

#: -> singleton(String)
def method3; T.unsafe(nil); end
T.reveal_type(method3) # error: Revealed type: `T.class_of(String)`
