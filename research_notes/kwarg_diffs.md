# Kwarg Naming Differences: .exp vs .prism.exp

## File 1: duplicate_kwarg.rb

### .exp (expected behavior)
```
def foo<<todo method>>(x:, x$1: = nil, &<blk>)
def bar<<todo method>>(x, y:, y$2: = nil, &<blk>)
```

### .prism.exp (Prism desugarer output)
```
def foo<<todo method>>(x:, x: = nil, &<blk>)
def bar<<todo method>>(x, y:, y: = nil, &<blk>)
```

### Difference
**Variable numbering is missing in Prism output:**
- `foo`: `x$1` becomes `x` (lost the `$1` suffix)
- `bar`: `y$2` becomes `y` (lost the `$2` suffix)

---

## File 2: fuzz_repeated_kwarg.rb

### .exp (expected behavior)
Contains variable numbering for duplicates:
- Line 10: `f3` has `x:, x$1:` (second kwarg is numbered)
- Line 14: `f4` has `x:, x$1: = nil` (second kwarg is numbered)
- Line 18: `f5` has `x, x$1:` (positional x, then kwarg x is numbered)
- Line 22: `f6` has `x, x$1: = nil`
- Line 26: `f7` has `x = 0, x$1: = nil`
- Line 30: `f8` has `this, this$1:` (this is numbered)

### .prism.exp (Prism desugarer output)
Missing variable numbering:
- Line 10: `f3` has `x:, x:` (no numbering)
- Line 14: `f4` has `x:, x: = nil` (no numbering)
- Line 18: `f5` has `x, x:` (no numbering)
- Line 22: `f6` has `x, x: = nil` (no numbering)
- Line 26: `f7` has `x = 0, x: = nil` (no numbering)
- Line 30: `f8` has `this, this:` (no numbering)

---

## Categorization: BUG

**This is a bug, not tolerable.**

### Reasoning

1. **Correctness**: Duplicate parameters in Ruby are a syntax error or need special handling. The numbering (`$1`, `$2`, etc.) is a desugaring mechanism to make them distinct internally when they appear in code.

2. **Inconsistency**: The original .exp files (from the previous parser) correctly number duplicate parameters. The Prism desugarer should preserve this behavior.

3. **Semantic Loss**: Removing the numbering creates ambiguous parameter names. If both parameters were named `x`, they become indistinguishable in the desugared tree.

4. **Pattern Scope**: This affects multiple functions across both test files, suggesting a systematic issue in how the Prism desugarer handles duplicate parameter names.

### Likely Root Cause

The Prism desugarer is not implementing the same parameter renaming/numbering logic that the original parser uses. When encountering duplicate parameter names, it should:
1. Keep the first occurrence as-is
2. Rename subsequent occurrences with `$N` suffix where N increments

This logic appears to be missing in the Prism desugarer.
