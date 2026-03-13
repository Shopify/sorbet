# typed: true
# disable-parser-comparison: true
foo = a(b
if @c && foo # error: unexpected token "if"
  d
end

if @d && foo
end
