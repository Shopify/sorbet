# Comparison: call_block_param.rb.desugar-tree-raw.exp vs .prism.exp

## File Paths
- **Legacy AST**: `test/prism_regression/call_block_param.rb.desugar-tree-raw.exp`
- **Prism AST**: `test/prism_regression/call_block_param.rb.desugar-tree-raw.prism.exp`

## Summary
Both files are almost identical. There are **no `$` variable numbering differences**. Instead, there are 3 semantic differences in how destructuring of rest parameters is desugared.

## Differences

### Location: Block parameter destructuring (lines ~517-597 in both files)
Context: The "block with multi-target rest args" case.

#### Change 1: `<expand-splat>` call argument (line 561)
**Legacy:**
```
Literal{ value = 1 }
```

**Prism:**
```
Literal{ value = 0 }
```

The second argument to `Magic.<expand-splat>` changes from `1` to `0`. This likely represents the minimum position to start splat expansion.

#### Change 2: Method call on expanded value (lines 577-582)
**Legacy:**
```
fun = <U []>
pos_args = 1
args = [
  Literal{ value = 0 }
]
```

**Prism:**
```
fun = <U to_ary>
pos_args = 0
args = [
]
```

The code switches from using `[]` indexing (accessor) to calling `to_ary`. This is a more idiomatic way to convert an object to an array.

## Variable Numbering
✓ **No issues**: `$2` through `$11` are consistently numbered in both files. No renumbering or gaps.

## Categorization: **BUG** (likely in Prism desugaring)

### Rationale
1. The `to_ary` method is semantically correct for converting a splatted parameter into an array
2. However, the change in the `<expand-splat>` first argument from `1` to `0` is suspicious—this changes the semantics of which parameters are considered "splat-able"
3. For a single rest parameter like `|*args|`, starting at position 0 is correct, but the legacy output (position 1) may indicate a bug in the original logic
4. The two changes are interdependent: if we're using position 0 for splat expansion, `to_ary` makes sense; if using position 1, the `[]` accessor is needed to skip the first element

### Next Steps
- Check the source Ruby code (`test/prism_regression/call_block_param.rb`) to understand what semantics are expected
- Verify the `<expand-splat>` behavior with position 0 vs 1 for rest parameters
- Determine if this affects runtime behavior or is a purely structural desugaring choice
