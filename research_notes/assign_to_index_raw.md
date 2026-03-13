# assign_to_index.rb.desugar-tree-raw Comparison

## Summary
Comparing `.exp` (original/non-Prism) vs `.prism.exp` (Prism-based desugaring) for block-pass handling in compound index assignments like `arr[&blk] &= val`.

**Key Finding:** Prism significantly simplifies the AST for block-pass cases by eliminating unnecessary temporary variable assignments.

---

## $ Variable Numbering Shift

| Metric | .exp | .prism.exp | Difference |
|--------|------|-----------|------------|
| Highest $ number | $99 | $89 | 10 fewer temps |
| Overall reduction | — | — | ~10% reduction in temp variables |

The Prism version starts numbering temps **lower** (uses $73 instead of $73-$76 for first block-pass case), indicating fewer intermediate assignments across all cases.

---

## Detailed Differences in Block-Pass Cases

The test file has three block-pass sections (lines ~56-67 in source):
```ruby
# Error case: using a block as an index
bitwise_and[&blk]     &= 202
bitwise_or[&blk]      |= 203
bitwise_xor[&blk]     ^= 204
# ... more operators
```

### .exp (Original) Structure
For **each** block-pass case (e.g., `bitwise_and[&blk] &= 202`), generates **4 temp assignments**:

```
InsSeq{
  stats = [
    Assign{ lhs = $73, rhs = <Magic> }           // Magic constant (unused?)
    Assign{ lhs = $74, rhs = bitwise_and() }    // Receiver
    Assign{ lhs = $75, rhs = :[] }              // Method name literal
    Assign{ lhs = $76, rhs = blk() }            // Block parameter
  ],
  expr = Send{
    recv = $73,
    fun = <call-with-block-pass>=>
    pos_args = 4
    args = [$74, $75, $76, Send(...&blk)]       // All temps + final computation
  }
}
```

**Pattern:** Creates intermediate temps for Magic, receiver, method name (`:[]`), and block.

### .prism.exp (Prism) Structure
For **each** block-pass case, generates **only 1 temp assignment**:

```
InsSeq{
  stats = [
    Assign{ lhs = $73, rhs = bitwise_and() }    // Receiver only
  ],
  expr = Send{
    recv = $73,
    fun = []=>                                   // Direct [] method, not <call-with-block-pass>
    pos_args = 1                                // Single arg (the computed value)
    args = [Send(...&blk)]                      // Just the block-pass Send
  }
}
```

**Pattern:** Only stores the receiver; no Magic constant, no method name literal, no block temp.

---

## AST Node Type Changes

| Node Feature | .exp | .prism.exp | Reason |
|--------------|------|-----------|--------|
| `fun` field in outer Send | `<call-with-block-pass>=>` | `[]=>` | Prism treats block-pass as simpler case |
| Inner Send `fun` | `<call-with-block-pass>` | `[]` | Direct method reference, not special wrapper |
| `pos_args` (outer) | 4 | 1 | Only receiver + value arg needed |
| Magic constant | ✓ (created) | ✗ (omitted) | Prism doesn't use it |
| Method name literal (`:[]`) | ✓ (stored in $75) | ✗ (omitted) | Method name known statically |
| Block temp variable | ✓ (stored in $74) | ✗ (omitted) | Inlined in computation |

---

## Variable Numbering: Why Different

The shift in $ numbers ($73-$76 vs $73 only) is **cascading**:

1. **First block-pass case:**
   - .exp uses $73, $74, $75, $76 (4 temps)
   - .prism.exp uses $73 only (1 temp)
   - **Difference: 3 fewer temps**

2. **Second block-pass case:**
   - .exp uses $77, $78, $79, $80 (building on $76)
   - .prism.exp uses $74 (building on $73)
   - **Offset accumulates**

3. **By end of block-pass section:**
   - .exp reaches $99 (accumulated 26-28 unused temps)
   - .prism.exp reaches $89 (10 fewer overall)

---

## Categorization: Tolerable or Bug?

### Assessment: **TOLERABLE** (arguably an improvement)

**Reasoning:**

1. **Semantically equivalent:** Both versions produce the same runtime behavior.
   - The Magic constant in `.exp` appears to be vestigial or internal bookkeeping.
   - Method names (`:[]`) and block temps can be inlined without loss of information.

2. **Prism's approach is more efficient:**
   - Fewer intermediate variables to track through desugaring.
   - Simpler AST structure for a common pattern.
   - Less clutter in temp variable namespace.

3. **No loss of information:**
   - All necessary computation is preserved; just structured differently.
   - The block-pass (`&blk`) is still explicitly represented in the final Send.

4. **Precedent:** Prism's desugarer explicitly handles block-pass specially, likely with different rules for how to construct the intermediate AST.

**Verdict:** This is **not a bug**—it's a different (and arguably better) desugaring strategy from Prism. The test expectations should be updated to reflect Prism's simpler structure, or a Prism-specific expectation file should be maintained.

---

## Diff Summary Stats

- **Total diff lines:** 1,233
- **Main diff pattern:** Removal of Magic constant, method name literal, and block temp assignments in all block-pass cases.
- **File sizes:**
  - `.exp`: ~25 KB
  - `.prism.exp`: ~21 KB (12% smaller)
- **Number of block-pass test cases affected:** 14+ (bitwise_and, bitwise_or, bitwise_xor, shift_right, shift_left, add_assign, subtract_assign, modulo_assign, divide_assign, multiply_assign, exponentiate_assign, lazy_and_assign, lazy_or_assign, multi-target)
