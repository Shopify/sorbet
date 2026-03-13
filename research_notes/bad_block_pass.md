# bad_block_pass.rb Test Comparison

## File Comparison

**Prism version** (`bad_block_pass.rb.desugar-tree.prism.exp`):
- Uses `::Magic.call-with-block-pass()` helper calls
- Desugars `&blk` into explicit calls to the magic helper

**Non-Prism version** (`bad_block_pass.rb.desugar-tree.exp`):
- Uses `::ErrorNode` as placeholder for invalid block pass nodes
- Does not have the magic helper call structure

## Line-by-Line Comparison

| Line | Prism Version | Non-Prism Version | Notes |
|------|------------------|---|---|
| 6-7 | `::<Magic>.<call-with-block-pass>(<self>.a(), :[]=, <self>.blk(), 1)` | `<self>.a().[]=(::<ErrorNode>, 1)` | Prism: explicit desugar with magic helper; Non-Prism: error node in arg |
| 8-9 | `::<Magic>.<call-with-block-pass>(<self>.a(), :[]=, <self>.blk(), 1)` | `<self>.a().[]=(::<ErrorNode>, 1)` | Same |
| 10 | `::<Magic>.<call-with-block-pass>(<self>, :a, <self>.blk(), 1)` | `<self>.a(::<ErrorNode>, 1)` | Different - regular call form |
| 12 | `::<Magic>.<call-with-block-pass>(<self>.a(), :[]=, <self>.blk(), 0, 1)` | `<self>.a().[]=(0, ::<ErrorNode>, 1)` | Two args before the bad block pass |
| 14 | `::<Magic>.<call-with-block-pass>(<self>, :a, <self>.blk(), 0, 1)` | `<self>.a(0, ::<ErrorNode>, 1)` | Two args, regular call form |

## Key Observations

### Variable Numbering
- No `$` variable numbering differences found
- Both versions maintain clean, direct desugaring without intermediate variables

### Structural Differences
The core difference is in how invalid block passes are handled:

1. **Prism approach**: Desugars the call structure explicitly using `::Magic.call-with-block-pass()` with:
   - The receiver/self
   - Method name (symbol)
   - The block (`<self>.blk()`)
   - The remaining arguments

2. **Non-Prism approach**: Leaves the `&blk` as an `::ErrorNode` in the argument position, preserving the original call structure but marking the invalid part as an error

## Categorization: **TOLERABLE**

This is a reasonable difference reflecting how Prism vs the older parser handle error recovery:

1. **Valid Use Case**: The test file intentionally contains syntax errors (unsupported block pass nodes). The test exists to verify error handling, not correct desugaring.

2. **Both Are Semantically Equivalent**: Both versions indicate that the code is invalid. The Prism version is more proactive in attempting a desugar structure (which might be useful for recovery), while the non-Prism version uses error nodes to mark problematic spots.

3. **No Correctness Bug**: Since the source code is invalid, there's no "correct" desugaring to compare against. Both approaches are valid ways to represent parse errors.

4. **Consistency Within Each**: Each version is internally consistent in how it handles the various bad block pass patterns.

## Recommendation

Keep the Prism version as-is. The differences are expected when migrating from one parser to another for error cases. If both produce reasonable error recovery behavior and help the user understand what went wrong, both are acceptable.
