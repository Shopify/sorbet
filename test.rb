# typed: strict

extend T::Sig

#: A
def parse_error1; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`

#: A
#  ^ test
def parse_error3; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`

#: A
#  ^ test
#
def parse_error2; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`

#: A
#  ^ test
#
#
def parse_error4; T.unsafe(nil); end # error: The method `parse_error1` does not have a `sig`
