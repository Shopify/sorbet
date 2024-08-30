# typed: false

# Regular assignment
Foo::REGULAR = 1

# Compound assignment operators
Foo::BITWISE_AND &= 2
Foo::BITWISE_XOR ^= 4
Foo::SHIFT_RIGHT >>= 5
Foo::SHIFT_LEFT <<= 6
Foo::SUBTRACT_ASSIGN -= 7
Foo::MODULE_ASSIGN %= 8
Foo::BITWISE_OR |= 9
Foo::DIVIDE_ASSIGN /= 10
Foo::MULTIPLY_ASSIGN *= 11
Foo::EXPONENTIATE_ASSIGN **= 12

# Special cases
Foo::LAZY_AND_ASSIGN &&= 13
Foo::LAZY_OR_ASSGIN ||= 14
