def dont_crash # error: does not have a `sig`
  x = begin; end || begin; end
  #   ^^^^^^^^^^ error: Left side of `||` condition was always `falsy`
  x = begin; end && begin; end
  #                 ^^^^^^^^^^ error: This code is unreachable
  x = () || ()
  #   ^^ error: Left side of `||` condition was always `falsy`
  x = () && ()
  #         ^^ error: This code is unreachable
end
