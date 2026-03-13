# Comparison of Prism vs Standard AST Expectations

## File 1: test/testdata/parser/misc.rb.desugar-tree.prism.exp vs .exp

### Variable Numbering Differences
The Prism version has **5 additional temporary variables** compared to the standard version:
- Lines 158-167 (Prism): Contains `$13` and `$14` (nested destructuring)
- Lines 158-163 (Standard): Only contains up to `$12`

### Details
- **Prism line 158-167**: Nested destructuring generates extra temporaries
  ```
  <assignTemp>$11 = <assignTemp>$10.[](1)
  <assignTemp>$12 = ::<Magic>.<expand-splat>(<assignTemp>$11, 1, 0)
  begin
    <assignTemp>$13 = <assignTemp>$12.[](0)
    <assignTemp>$14 = ::<Magic>.<expand-splat>(<assignTemp>$13, 1, 0)
    x = <assignTemp>$14.[](0)
    <assignTemp>$13
  end
  ```

- **Standard line 158-163**: Simpler desugaring
  ```
  <assignTemp>$11 = <assignTemp>$10.[](1)
  <assignTemp>$12 = ::<Magic>.<expand-splat>(<assignTemp>$11, 1, 0)
  x = <assignTemp>$12.[](0)
  <assignTemp>$11
  ```

### Other Differences (Non-$)
- Line 181 (Standard) vs Line 186 (Prism): `"5"` vs `"5r"` - Rational literal representation
- Line 212 (Standard): `<self>.proc() do |x|` vs Line 217 (Prism): `<self>.proc() do ||` - Block parameter difference

### Categorization
- **$13, $14 numbering**: **BUG** - Prism generates unnecessary extra nesting for destructuring that the standard parser avoids
- **"5r" vs "5"**: **TOLERABLE** - Just different representation of the same value
- **Block parameter (|x| vs ||)**: **BUG** - Semantic difference; Prism loses parameter information

---

## File 2: test/testdata/rbs/assertions_heredoc_modified.rb.rewrite-tree.prism.exp vs .exp

### Summary
No $ variable numbering differences detected.

### Differences Present
- **Lines 10-56** (Prism): Contains wrapping with `<cast:let>()` nodes
- **Lines 10-56** (Standard): Direct assignment without cast nodes

### Examples
**Standard (line 10)**:
```
<emptyTree>::<C HEREDOC4> = "foo\n"
```

**Prism (line 10)**:
```
<emptyTree>::<C HEREDOC4> = <cast:let>("foo\n", <todo sym>, ::<root>::<C T>.nilable(<emptyTree>::<C String>))
```

### Categorization
- **<cast:let> wrapping**: **TOLERABLE** - This appears to be type annotation/type tracking metadata added by the Prism rewriter. The actual value and logic remain the same.

---

## File 3: test/testdata/resolver/sig_misc.rb.symbol-table-raw.prism.exp vs .exp

### Summary
No $ variable numbering differences detected. Structure is identical except for location offsets.

### Differences Present
All differences are line number shifts in location annotations:
- Prism file shows line numbers 1 position higher across the board
  - Line 3 (Standard) → Line 3 (Prism) in static-init location
  - But file line references differ: `start=3:1 end=117:4` (Prism) vs `start=2:1 end=116:4` (Standard)

### Categorization
- **Line number offsets**: **TOLERABLE** - Simple source file offset; likely due to file structure differences (e.g., shebang or encoding declaration)

---

## File 4: test/testdata/rewriter/minitest_empty_test_each.rb.rewrite-tree.prism.exp vs .exp

### Summary
No $ variable numbering differences detected. Almost identical.

### Differences Present
- **Prism (line 44)**: Contains `::<ErrorNode>` at end
- **Standard (lines 43-44)**: Missing error node, ends cleanly

### Categorization
- **::<ErrorNode>**: **BUG** - Prism tree has error marker that standard doesn't. The expectation files should match or this indicates a real parsing issue with Prism on this input.

---

## Summary by Category

### Bugs (require fixing)
1. **misc.rb**: Extra temp variables $13, $14 in nested destructuring (Prism over-generates)
2. **misc.rb**: Block parameter lost in proc block (|x| vs ||)
3. **minitest_empty_test_each.rb**: Unexpected ErrorNode in Prism output

### Tolerable (expected differences)
1. **misc.rb**: Rational literal representation ("5r" vs "5")
2. **assertions_heredoc_modified.rb**: Type annotation wrapping with `<cast:let>`
3. **sig_misc.rb**: Line number offsets in location info
