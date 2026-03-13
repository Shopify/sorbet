# Error Recovery Test Comparison: Prism vs Non-Prism

This document compares the desugar-tree output between Prism-specific (`.desugar-tree.prism.exp`) and non-Prism (`.desugar-tree.exp`) test expectations for error recovery test files.

## Summary of Differences

Most files show significant structural and semantic differences between the two parsers' error recovery behavior. The key differences are:

1. **Prism parser tends to wrap code in `begin` blocks** when handling certain error conditions
2. **Different handling of invalid syntax** - Prism produces `::<ErrorNode>` inline, while non-Prism wraps them in class constants like `<emptyTree>::<C <ErrorNode>>`
3. **Variable numbering**: Both use `$2`, `$3`, etc., but they're used in different contexts
4. **Block argument handling**: Prism generates plain identifiers (e.g., `|x|`) while non-Prism converts them (e.g., `||`)

---

## File-by-File Analysis

### begin_1.rb
**Prism**: Wraps code in explicit `begin...end` blocks with `rescue` clauses
**Non-Prism**: Puts rescue clauses directly on the `def` statements without extra begin blocks
- Structure: Prism nests begin/rescue deeply, Non-Prism flattens them to method-level rescues

### block_do_1.rb
**Prism**: Wraps code in `begin...end` blocks
**Non-Prism**: Direct `begin...end` blocks inside methods only
- Nesting depth differs significantly

### block_forwarding_invalid_def.rb
**Files are VERY DIFFERENT**
- **Prism**: Complex multi-line desugaring with `case9_block_param_and_forwarding`, `case10_anonymous_block_param_and_forwarding`, etc. with `*<fwd-kwargs>:` syntax
- **Non-Prism**: Simple constant assignment - just `"literal block"`
- This is a major semantic difference, not just formatting

### case_1.rb
**Prism**: Complex nested `begin...if...end` structures with temporary variables
**Non-Prism**: Same nesting but different handling of empty tree nodes and error nodes

### case_2.rb
**Prism**: Generates `<assignTemp>$2 = <self>.x()` and class wrapping
**Non-Prism**: Different error node handling (`<emptyTree>::<C <ErrorNode>>` vs `::<ErrorNode>`)

### circular_argument_reference.rb
**Prism**: Block wrapping with nested def statements inside begin blocks
**Non-Prism**: Flattened structure with separate methods
- Prism keeps nested structure; Non-Prism breaks them apart

### def_missing_end_1.rb
**Prism**: Wraps test1 method body in explicit `begin...end`
**Non-Prism**: Direct if statement without extra begin block
- Prism adds wrapping, Non-Prism doesn't

### def_missing_end_2.rb
**Prism**: test2 wrapped in `begin...end`
**Non-Prism**: test2 has direct begin...end (similar but slightly different organization)

### forward_args.rb
**Prism**: Uses `_1` for numbered parameter in block
**Non-Prism**: Uses `|_1|` explicit parameter naming
- Difference in how numbered parameters are handled

### missing_operator.rb
**Prism**: More compact, inline `begin...if` expressions
**Non-Prism**: Many more separate statements at module/class level, not wrapped in method bodies
- Major structural difference: Prism wraps more in methods

### numparams.rb
**Prism**: Parameter named `x` in block, but body references `_1` (numbered param conflict)
**Non-Prism**: Parameter correctly named `_1` in block signature
- Parameter naming mismatch in Prism

### numparams_crash_1.rb
**Prism**: Block parameter `|x|` but body uses `_1`
**Non-Prism**: Block parameter `|_1|` and body uses `_1`
- Similar parameter mismatch

### numparams_crash_2.rb
**Prism**: `f(1) do |x|` with body `_1`
**Non-Prism**: `f(1) do |_1|` with body `_1`
- Block parameter naming conflict

### numparams_crash_3.rb
**Prism**: Block has no parameters `do ||` but contains error node
**Non-Prism**: Block has no parameters `do ||` and bar symbol
- Prism includes `::<ErrorNode>` inline, Non-Prism skips it

### numparams_crash_5.rb
**Prism**: `map() do |x|` with body `_1`
**Non-Prism**: `map() do |_1|` with body `_1`
- Parameter naming issue again

### other_missing_end.rb
**Prism**: Deeply nested begin blocks inside method definitions
**Non-Prism**: Flattened method definitions with proper begin...end wrapping at method level
- Significant structural nesting difference

### pos_after_kw_1.rb
**Prism**: Multiple `::<ErrorNode>` markers in sig block
**Non-Prism**: Cleaner chain of `.params(...).returns(...)`
- Prism preserves error nodes explicitly

### unmatched_block_args_1.rb
**Prism**: Multiple block argument names preserved (`|puts|`, `|Opus|`, `|x, puts|`)
**Non-Prism**: Converts to `||` (no args) for mismatched cases
- Prism keeps declared args even when wrong; Non-Prism strips them

### unmatched_block_args_2.rb
**Prism**: Same as _1 - keeps declared args like `|puts|` and `|Opus|`
**Non-Prism**: Converts to `||` for errors
- Same pattern as _1

### unmatched_block_args_3.rb
**Prism**: Keeps block args but with one method pruned (`test_two_args` differs)
**Non-Prism**: Converts to `||` consistently
- Variation in how many methods/blocks are affected

### unmatched_block_args_4.rb
**Prism**: When block args fail, uses error recovery to create `.x()` call wrapper
**Non-Prism**: Same wrapping pattern with `||` blocks
- Interesting alternative error recovery: wraps invalid args as call arguments

### unterminated_array.rb
**Prism**: Inline `::<ErrorNode>` in arrays, like `[1, <self>.puts(), "hi", ::<ErrorNode>]`
**Non-Prism**: Wraps error node as class reference `[<emptyTree>::<C <ErrorNode>>]`
- Different error node representation

### unterminated_array_bad_1.rb
**Prism**: Complex nested structure with `.sig()` call and method def inside array
**Non-Prism**: Simple array with error node `[<emptyTree>::<C <ErrorNode>>]`
- Prism includes actual parsed content; Non-Prism treats as single error

### unterminated_array_bad_2.rb
**Prism**: Array literal with inline puts and error node
**Non-Prism**: Array with just error node
- Prism preserves more of the parsed content

---

## Key Patterns

### $ Variable Numbering
All files use `$2`, `$3`, etc. consistently. Notable uses:
- `<assignTemp>$2` - temporary variable for assignment
- `<rescueTemp>$2` - temporary variable for rescue
- `&&$2`, `||$2` - for logical operator desugaring
- These numbering patterns are consistent between Prism and Non-Prism

### Error Node Representation
- **Prism**: `::<ErrorNode>` (simple error marker)
- **Non-Prism**: `<emptyTree>::<C <ErrorNode>>` (error node wrapped in empty tree class)

### Block Parameter Handling
Major difference in how invalid block parameters are handled:
- **Prism**: Preserves the declared parameter names even when they're invalid
- **Non-Prism**: Strips invalid parameters and uses `||` (empty parameter list)

### Structure Wrapping
- **Prism**: Wraps more code in `begin...end` blocks
- **Non-Prism**: Keeps structure flatter, rescues at method level only

---

## Recommendations

For tests to pass on Prism, the expectations files need to account for:
1. Error node representation differences
2. Block parameter handling divergence
3. Different levels of begin block wrapping
4. Structural differences in how error recovery preserves vs. strips invalid syntax

The variable numbering ($2, $3, etc.) is not a major issue - both parsers seem consistent there.
