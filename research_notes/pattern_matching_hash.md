# Pattern Matching Hash Comparison

## Files Compared
- `test/testdata/desugar/pattern_matching_hash.rb.desugar-tree.prism.exp` (Prism output)
- `test/testdata/desugar/pattern_matching_hash.rb.desugar-tree.exp` (Original output)

## Key Differences

### 1. Temporary Variable Numbering
**Line 5:**
- Prism: `<assignTemp>$4 = expr`
- Original: `<assignTemp>$3 = expr`

This is the ONLY `$` variable numbering difference.

### 2. Variable Assignments in Fourth Pattern Match (Lines 29-40)

**Prism Output (Lines 29-43):**
```
begin
  <emptyTree>
  ::<ErrorNode>
  {:kg => <self>.g()}
  ::<ErrorNode>
  ::<ErrorNode>
  <self>.h()
  ::<ErrorNode>
  ::<ErrorNode>
  <self>.i()
  <emptyTree>::<C T>.reveal_type(e)
  <emptyTree>::<C T>.reveal_type(<self>.g())
  <emptyTree>::<C T>.reveal_type(<self>.h())
  <emptyTree>::<C T>.reveal_type(<self>.i())
end
```

**Original Output (Lines 29-40):**
```
begin
  i = ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
  e = ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
  g = ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
  h = ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
  begin
    <emptyTree>::<C T>.reveal_type(e)
    <emptyTree>::<C T>.reveal_type(g)
    <emptyTree>::<C T>.reveal_type(h)
    <emptyTree>::<C T>.reveal_type(i)
  end
end
```

## Analysis

### Temporary Variable ($4 vs $3)
**Category: TOLERABLE**

The difference in temporary variable numbering ($4 vs $3) is expected given that Prism's desugaring pass likely handles pattern matching differently. The actual temp variable number is an implementation detail that shouldn't affect correctness as long as there are no conflicts. This is likely due to different intermediate steps in the desugaring process.

### Variable Assignments (e, g, h, i)
**Category: BUG**

This is a significant structural difference:
- The original output properly assigns variables `e`, `g`, `h`, `i` from pattern matching destructuring
- The Prism output produces `<ErrorNode>` markers and method calls (`<self>.g()`, `<self>.h()`, `<self>.i()`) instead of proper variable assignments
- The `reveal_type` calls in Prism reference the method calls directly rather than the assigned variables

This suggests that Prism's pattern matching desugaring for hash patterns with method calls as keys is not correctly extracting and binding the matched values. The code should be generating variable assignments, not error nodes and method invocations.

## Summary

| Difference | Category | Reason |
|-----------|----------|--------|
| `$4` vs `$3` temp variable | TOLERABLE | Implementation detail; different intermediate desugaring steps |
| Missing variable assignments (e, g, h, i) | BUG | Prism output fails to properly extract pattern-matched values; generates ErrorNodes instead |
