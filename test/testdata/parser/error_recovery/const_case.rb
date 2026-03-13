# typed: false
# disable-parser-comparison: true

def foo(x)
  Integer::
 #       ^^ error: expected constant name following "::"
  case x
  when Integer
  end
end
