# typed: false

begin
  123
rescue
  "rescued"
end

begin
  123
rescue FooException
#      ^^^^^^^^^^^^ error: Unable to resolve constant `FooException`
  "rescued Foo"
end

begin
  123
rescue FooException
  "rescued Foo"
rescue BarException
  "rescued Bar"
end
