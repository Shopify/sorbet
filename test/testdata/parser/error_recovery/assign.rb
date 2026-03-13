# typed: false
# disable-parser-comparison: true
# Should still see at least method def (not body)
def test_bad_assign(x)
  x =
end # error: unexpected token
