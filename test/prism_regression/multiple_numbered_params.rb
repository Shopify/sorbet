# typed: false
# disable-parser-comparison: true

def f(p)
  RATCHET.any? { p.dir == _1 || p.dir.start_with?(_1 + "/") }
end
