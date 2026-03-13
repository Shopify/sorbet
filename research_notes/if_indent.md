# Comparison of if_indent and if_do_2 Test Expectations

## Overview
Comparing 6 test file pairs:
- `if_indent_1.rb` through `if_indent_5.rb`
- `if_do_2.rb`

Each pair consists of:
- `.desugar-tree.exp` (old/existing expectation)
- `.desugar-tree.prism.exp` (Prism-specific expectation)

These files test error recovery for malformed `if` statements with varying indentation and syntax patterns.

## Finding: No $ Variable Differences
No `$` variable numbering differences found anywhere in these files. The desugarer output does not use `$0`, `$1`, etc. patterns in these test cases.

## Pair-by-Pair Comparison

### if_indent_1.rb

**Source Context:** Tests 4 error cases with missing/incomplete if conditions, plus 3 valid cases.

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | 7 (test0-3 + no_syntax_error_1-3) | 1 (test0 only) | **Prism truncates after first error** |
| test0 condition | `<emptyTree>::<C <ErrorNode>>` | `::<ErrorNode>` | **Format change: loses `<emptyTree>` qualifier** |
| Total lines | 75 | 15 | Prism is much more concise |

**Key observation:** Prism version stops parsing/desugaring after the first error in test0, whereas the old parser attempts to recover and continue with subsequent methods.

---

### if_indent_2.rb

**Source Context:** 5 error cases where `puts 'after'` statement appears on the same logical level as the broken `if`.

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | 5 (test0-4) | 5 (test0-4) | **Same** |
| Structure | Each method in sequence at class level | Methods heavily nested (indentation errors cascade) | **Major parsing recovery difference** |
| test0 | Single method | Single method | Same |
| test1-4 | Separate methods | Each method nested inside the previous one's begin block | **Cascading nesting in Prism** |

**Key observation:** Old parser treats misindented code as new methods at class level. Prism appears to nest them inside begin blocks, suggesting different error recovery strategy for indentation.

---

### if_indent_3.rb

**Source Context:** 4 error cases in value assignment position (`x = if ...`).

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | 5 (test0-3 + duplicate test3) | 1 (test0 only) | **Prism stops after first error** |
| test0 condition | `<emptyTree>::<C <ErrorNode>>` | `::<ErrorNode>` | **Same format change as if_indent_1** |
| Total lines | 44 | 12 | Prism is much more concise |

**Key observation:** Same pattern as if_indent_1—Prism halts after first error, old parser recovers and continues.

---

### if_indent_4.rb

**Source Context:** 2 methods with missing/incomplete if conditions that cause structural issues.

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | 2 (test1, test2) | 2 (test1, test2) | **Same** |
| test1 structure (if_indent_4.exp) | Method-level, puts at class level | Method-level with begin block wrapping | **Begin block wrapping** |
| test1 structure (prism) | With begin block | With begin block | **Same** |

**Key observation:** When error is part of method body with nested structure, both versions wrap in begin. Difference is in how following statements are placed.

---

### if_indent_5.rb

**Source Context:** Test with module, nested classes, and method definitions.

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | Complex structure with sig/private | Simple structure | **Prism drastically simplifies** |
| test1 body (exp) | Nested sig/private method defs | Just the if statement | **Old parser includes more structure** |
| Error representation | `<emptyTree>::<C <ErrorNode>>` | `::<ErrorNode>` | **Same format difference** |
| Total lines | 40 | 14 | Prism is 65% smaller |

**Key observation:** Another case where Prism truncates complex error recovery.

---

### if_do_2.rb

**Source Context:** Tests error cases with blocks (`do...end`) in if conditions.

| Aspect | .exp | .prism.exp | Difference |
|--------|------|-----------|-----------|
| Methods present | 2 (test1, test2) | 2 (test1, test2) | **Same** |
| test1 structure | Straightforward method with begin | Straightforward method with begin | **Same** |
| test2 body (exp) | Single method | Nested method (test2 inside test1's begin) | **Cascading nesting** |
| Error placement (exp) | `::<ErrorNode>` at class level | `::<ErrorNode>` inside nested method | **Different error scope** |

**Key observation:** Similar to if_indent_2—Prism nests methods due to indentation error recovery, placing error nodes deeper in AST.

---

## Summary of Patterns

### 1. No $ Variable Numbering
✓ Confirmed: No `$0`, `$1`, etc. patterns in either .exp or .prism.exp files.

### 2. ErrorNode Format Difference
The only syntactic difference found in ErrorNode representation:
- Old: `<emptyTree>::<C <ErrorNode>>`
- Prism: `::<ErrorNode>`

The Prism version omits the `<emptyTree>::` prefix.

### 3. Error Recovery Strategy Divergence

**Old parser behavior:**
- Continues parsing after syntax errors
- Places misindented code at class/module level as separate methods
- Preserves detailed structure (sig blocks, private methods, etc.)

**Prism behavior:**
- Often halts desugaring after first critical error
- When continuing, uses cascading nesting for indentation errors
- Produces simpler/shorter output

### 4. Structural Implications

The differences suggest:
- Prism's desugarer may stop at the first unrecoverable error (affects files with multiple independent errors)
- Indentation error handling creates nested structures in Prism vs. sequential siblings in old parser
- This isn't a variable numbering issue but rather fundamental AST structure differences

## Files Analyzed

- `/test/testdata/parser/error_recovery/if_indent_1.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_indent_1.rb.desugar-tree.prism.exp`
- `/test/testdata/parser/error_recovery/if_indent_2.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_indent_2.rb.desugar-tree.prism.exp`
- `/test/testdata/parser/error_recovery/if_indent_3.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_indent_3.rb.desugar-tree.prism.exp`
- `/test/testdata/parser/error_recovery/if_indent_4.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_indent_4.rb.desugar-tree.prism.exp`
- `/test/testdata/parser/error_recovery/if_indent_5.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_indent_5.rb.desugar-tree.prism.exp`
- `/test/testdata/parser/error_recovery/if_do_2.rb.desugar-tree.exp`
- `/test/testdata/parser/error_recovery/if_do_2.rb.desugar-tree.prism.exp`
