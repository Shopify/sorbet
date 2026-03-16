# typed: strict
# enable-experimental-rbs-comments: true
# Modified for Prism: csend desugaring assigns inner/outer temp variables in
# opposite order ($10/$11 swapped), producing semantically equivalent but
# textually different rewrite-tree output.

this = self #: as untyped

this&.puts #: as String

this&.ARGV.first #: as String

begin
  ARGV #: as Integer?
end&.first # error: Method `first` does not exist on `Integer`

this&.puts begin
  ARGV.first #: as String
end #: as String

this&.puts(
  42 #: as String
)

this&.puts(
  42, #: as String
  42 #: as String
)

this&.
  puts #: as Integer

this&. #: as Integer?
  first #: as Integer
# ^^^^^ error: Method `first` does not exist on `Integer`

ARGV #: as Integer?
 &.first #: as String? # error: Method `first` does not exist on `Integer`
 &.last # error: Method `last` does not exist on `String`
