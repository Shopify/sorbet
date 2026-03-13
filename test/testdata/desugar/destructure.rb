# typed: true
# disable-parser-comparison: true
class Destructure
  def f((x,y), z)
    x + y

    lambda do |(a,b)|
    end
  end
end
