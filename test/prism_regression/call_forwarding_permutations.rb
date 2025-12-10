# typed: false

# This file tests combinations of block forwarding and literal blocks.
#
# Summary of 8 combinations:
#   Valid:
#     1. None           - def f; bar; end
#     2. Just &         - def f(&b); bar(&b); end
#     3. Just ...       - def f(...); bar(...); end
#     4. Just { }       - def f; bar { }; end
#     5. ... + { }      - def f(...); bar(...) { }; end
#   Error cases (call-site):
#     7. & + { }        - def f(&b); bar(&b) { }; end  -- prism crashes
#   Error cases (definition - method lost in error recovery):
#     6. & + ...        - def f(&, ...); end
#     8. & + ... + { }  - def f(&, ...); bar(...) { }; end

# ==============================================================================
# VALID CASES - These should produce identical output
# ==============================================================================

# Case 1: None
def case1_none
  bar
end

# Case 2: Just & - Explicit block parameter
def case2_block_param(&block)
  bar(&block)
end

# Case 2a: Anonymous block parameter
def case2a_anonymous_block_param(&)
  bar(&)
end

# Case 3: Just ... - Forwarding only
def case3_forwarding(...)
  bar(...)
end

# Case 4: Just { } - Block literal only
def case4_block_literal
  bar { "block body" }
end

# CASE 5: ... + { } at call site
# Forwarding includes <fwd-block>, AND there's a literal block.
# Both should be kept in the output.
def case5_forwarding_and_literal(...)
  foo(...) { "literal block" }
end

# ==============================================================================
# EXCLUDED CASES - Commented out because they crash prism or lose the method
# ==============================================================================

# Case 6: & + ... in definition (named block param)
# Parser error: can't have both & and ... in definition.
# Method definition is LOST in error recovery - only inner expressions survive.
# def case6_block_param_and_forwarding(&block, ...)
#   bar(...)
# end

# Case 6a: & + ... in definition (anonymous block param)
# Same issue - method is lost.
# def case6a_anonymous_block_param_and_forwarding(&, ...)
#   bar(...)
# end

# Case 7: & + { } at call site (named block param)
# Prism crashes with: "PM_BLOCK_ARGUMENT_NODE is handled specially in desugarArguments()"
# def case7_block_pass_and_literal(&block)
#   foo(&block) { "literal block" }
# end

# Case 7a: & + { } at call site (anonymous block param)
# Same crash.
# def case7a_anonymous_block_pass_and_literal(&)
#   foo(&) { "literal block" }
# end

# Case 8: & + ... + { } (all three, named block param)
# Parser error at definition causes method to be lost.
# def case8_all_three(&block, ...)
#   foo(...) { "literal block" }
# end

# Case 8a: & + ... + { } (all three, anonymous block param)
# Same issue - method is lost.
# def case8a_all_three_anonymous(&, ...)
#   foo(...) { "literal block" }
# end
