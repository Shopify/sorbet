# typed: true
# disable-parser-comparison: true
module Model # error: module definition in method body
  def a-b; end # error: unexpected token
end
