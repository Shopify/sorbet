# typed: false

def foo
  foobar
  problematic_code
rescue
  puts "rescued"
end

def bar
  problematic_code
rescue RuntimeError => e
  puts "rescued"
end

begin
  problematic_code
rescue => e
  puts "rescued"
end

begin
  problematic_code
rescue FooException => e
  puts "rescued foo"
rescue BarException => e
  puts "rescued bar"
else
  puts "not rescued"
end

foo rescue puts "rescued"
problematic_code rescue nil
problematic_code rescue raise rescue puts "rescued again"
