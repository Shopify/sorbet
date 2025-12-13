# typed: false
# disable-parser-comparison: true

def f(x)
  x.map(&:field)
end
