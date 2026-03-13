# typed: true
# disable-parser-comparison: true

class A
  def example
    example{&:"{}"}
    #      ^^^^^^^^ error: block pass should not be enclosed in curly braces
  end
end
