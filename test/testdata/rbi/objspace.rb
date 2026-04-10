# typed: true

class Parent; end
module Mixin; end

ObjectSpace.each_object do |obj|
  T.reveal_type(obj) # error: `BasicObject`
end
ObjectSpace.each_object(Parent) do |obj|
  T.reveal_type(obj) # error: `Parent`
end
ObjectSpace.each_object(Mixin) do |obj|
  T.reveal_type(obj) # error: `Mixin`
end

res = ObjectSpace.each_object
T.reveal_type(res) # error: `T::Enumerator[BasicObject]`
res = ObjectSpace.each_object(Parent)
T.reveal_type(res) # error: `T::Enumerator[Parent]`
res = ObjectSpace.each_object(Mixin)
T.reveal_type(res) # error: `T::Enumerator[Mixin]`

# Test WeakKeyMap generics
weak_map = ObjectSpace::WeakKeyMap[String, Integer].new
T.reveal_type(weak_map) # error: `ObjectSpace::WeakKeyMap[String, Integer]`

# Test key/value access
weak_map["key"] = 42
value = weak_map["key"]
T.reveal_type(value) # error: `T.nilable(Integer)`

# Test delete
deleted = weak_map.delete("key")
T.reveal_type(deleted) # error: `T.nilable(Integer)`

# Test getkey
key = weak_map.getkey("some_key")
T.reveal_type(key) # error: `T.nilable(String)`

# Test key? method
exists = weak_map.key?("key")
T.reveal_type(exists) # error: `T::Boolean`

# Test clear method
cleared = weak_map.clear
T.reveal_type(cleared) # error: `ObjectSpace::WeakKeyMap[String, Integer]`
