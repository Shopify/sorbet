# typed: true
# disable-parser-comparison: true

a = nil
b = 1
c = ""

case nil
in [^a]
in {b: ^b}
in ^c
end
