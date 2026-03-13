# typed: false
# disable-parser-comparison: true
class A; end
def test_constant_only_scope
  A::
  #^^ error: expected constant name following "::"
end
