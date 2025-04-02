# typed: strict
# enable-experimental-rbs-signatures: true

#: (P1, P2
#| -> void
#  ^^ error: Failed to parse RBS signature (unexpected token for function parameter name)
def parse_error1; T.unsafe(nil); end # error: The method `parse_error2` does not have a `sig`

#: ->
#| String
def method1; T.unsafe(nil); end

#: (
#|   Integer
#| ) -> String
def method2; T.unsafe(nil); end

#: (
#|   Integer,
#|   String
#| )
#| ->
#| String
def method3; T.unsafe(nil); end

#:(
#|Integer,
#|String
#|) ->
#|String
def method4; T.unsafe(nil); end
