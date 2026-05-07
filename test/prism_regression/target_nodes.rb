# typed: false

begin
rescue => @ivar
end

begin
rescue => @@cvar
end

begin
rescue => $gvar
end

begin
rescue => self.call_target1
end

begin
rescue => self::call_target2
end

begin
rescue => ConstantTarget
end

begin
rescue => ::ConstantPathTarget
end

begin
rescue => Nested::ConstantPathTarget
end

begin
rescue => ::Nested::ConstantPathTarget
end

begin
rescue => a[123]
end
