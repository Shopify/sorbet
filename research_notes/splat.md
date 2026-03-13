# Splat.rb Desugaring Comparison: Prism vs Non-Prism

## Files Compared
- `test/testdata/desugar/splat.rb.desugar-tree.prism.exp` (Prism desugarer)
- `test/testdata/desugar/splat.rb.desugar-tree.exp` (Non-Prism desugarer)

## Key Difference

The only difference appears on **line 55**:

### Prism Version (line 55)
```
rescue <emptyTree>::<C Array>, ::<Magic>.<splat>(<emptyTree>::<C Rescueable>.new()), <emptyTree>::<C Float> => <rescueTemp>$4
```

### Non-Prism Version (line 55)
```
rescue [<emptyTree>::<C Array>].concat(::<Magic>.<splat>(<emptyTree>::<C Rescueable>.new())).concat([<emptyTree>::<C Float>]) => <rescueTemp>$4
```

## Variable Numbering
Both versions use the same `$` variable numbering:
- `<assignTemp>$2` (line 44)
- `<rescueTemp>$3` (line 52)
- `<rescueTemp>$4` (line 55)

**No differences in variable numbering between the two versions.**

## Analysis

This is a **structural difference in how rescue clause arguments are desugared**, not a variable numbering issue.

### The Difference Explained
The non-Prism version explicitly constructs an array from multiple rescue exception classes using `.concat()` chaining:
```
[<C Array>].concat(<splat>...).concat([<C Float>])
```

The Prism version represents the rescue clause as a flat comma-separated list of exception types:
```
<C Array>, <splat>(...), <C Float>
```

### Categorization: **TOLERABLE**

**Reasoning:**
1. Both forms are semantically equivalent - they're rescue clauses that catch from multiple exception types
2. The variable numbering is identical and consistent
3. The difference is in the internal representation strategy:
   - Non-Prism: builds an array structure explicitly (likely to match how the AST parser sees it)
   - Prism: uses a cleaner comma-separated representation (more direct encoding of the source)
4. This is a desugaring style choice, not a bug - both would execute identically

The Prism version is arguably cleaner/simpler. The non-Prism version with explicit array concatenation appears to be an artifact of how that parser constructs the rescue argument structure.
