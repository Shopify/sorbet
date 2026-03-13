# Comparison: star_in_block_arg.rb.desugar-tree.prism.exp vs .exp

## Summary
Two key differences found in lines 16-17. Variable numbering ($2, $3, $4) is identical.

## Detailed Diff

### Line 16: expand-splat arguments
- **Prism**: `<assignTemp>$4 = ::<Magic>.<expand-splat>(<assignTemp>$3, 0, 0)`
- **AST**: `<assignTemp>$4 = ::<Magic>.<expand-splat>(<assignTemp>$3, 1, 0)`

The second argument to `expand-splat` differs: `0` vs `1`

### Line 17: array element access
- **Prism**: `args = <assignTemp>$4.to_ary()`
- **AST**: `args = <assignTemp>$4.[](0)`

Prism calls `.to_ary()` while AST indexes with `.[](0)`

## Analysis

### Dollar Variable Numbering
All variable IDs ($2, $3, $4) are identical between both versions. **No numbering conflicts.**

### Difference Categorization

**Line 16 - `expand-splat` argument difference:**
- The first numeric argument (0 vs 1) likely indicates a flag or mode parameter
- Prism uses 0, AST uses 1
- This suggests different desugaring strategies for handling splat expansion
- **Assessment**: Potentially a **bug** — if both should be producing equivalent desugared output, they should use the same expand-splat mode

**Line 17 - array extraction method:**
- Prism: `.to_ary()` — converts result to array
- AST: `.[](0)` — indexes into result at position 0
- This is a functional difference in how the splat result is extracted
- **Assessment**: **Bug** — These produce different semantics:
  - `.to_ary()` returns the full array
  - `.[](0)` returns only the first element
  - For a block parameter with splat (`|*args|`), we need the full array, not just the first element

## Conclusion
Both differences appear to be **bugs in the Prism desugarer**. The AST version correctly:
1. Uses expand-splat mode 1 (likely the correct mode for this context)
2. Extracts the full array result with `.to_ary()` rather than indexing

The Prism version would likely fail at runtime or produce incorrect behavior with multiple block arguments.
