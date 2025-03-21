# typed: strict
# enable-experimental-rbs-signatures: true
# enable-experimental-rbs-assertions: true

argv = ARGV #: Array[String]

as1 = argv.first #:as !nil
T.reveal_type(as1) # error: Revealed type: `String`

as2 = argv.first #: as !nil
T.reveal_type(as2) # error: Revealed type: `String`

as3 = argv.first #:      as      !nil
T.reveal_type(as3) # error: Revealed type: `String`

as4 = argv.first #: as !nil #some comment
T.reveal_type(as4) # error: Revealed type: `String`

as5 = argv.first #: as !nil # some comment
T.reveal_type(as5) # error: Revealed type: `String`
