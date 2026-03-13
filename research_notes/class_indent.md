# Class Indentation Error Recovery: Prism vs Legacy Parser Desugar Tree Comparison

## Overview
Compared 7 test file pairs for desugar-tree output differences between Prism parser (`.prism.exp`) and legacy parser (`.exp`):
- `class_indent_1.rb` through `class_indent_6.rb` (6 pairs)
- `class_weird_newline_indent.rb` (1 pair)

These tests exercise error recovery around malformed/indented class definitions.

## Key Findings

### No $ Variable Numbering Differences
There are **no `$` variable numbering differences** between the Prism and legacy outputs. Both use identical variable numbering schemes.

### Primary Pattern: ErrorNode Placement

The **consistent pattern of difference** is the placement of `::ErrorNode` markers:

**Prism version (.prism.exp):**
- Places `::ErrorNode` where it logically belongs in the scope (inside the class where parsing failed)
- Appears at the end of the malformed class body

**Legacy version (.exp):**
- Often omits `::ErrorNode` entirely
- When included, places it differently in the tree structure

### Detailed Breakdown by Test Case

#### class_indent_1.rb
- **Prism:** ErrorNode at line 9 (inside inner class, where the class failed to close properly)
- **Legacy:** No ErrorNode; structure cleaner but less diagnostic

**Difference:** Prism correctly identifies the `class Inner` didn't close and marks error at the point of failure

#### class_indent_2.rb
- **Prism:** ErrorNode at line 13 (marks failed inner class context)
- **Legacy:** No ErrorNode
- Both include the two `puts 'hello'` statements, but differently scoped

**Difference:** Prism's ErrorNode more clearly indicates parsing state when `Inner` class wasn't properly closed

#### class_indent_3.rb
- **Prism:** ErrorNode at line 13 (failed inner class)
- **Legacy:** No ErrorNode

**Pattern continues:** ErrorNode marks the scope where closure failed

#### class_indent_4.rb
- **Prism:** ErrorNode at line 15 (failed inner class)
- **Legacy:** No ErrorNode

#### class_indent_5.rb
- **Prism:** ErrorNode at line 15 (marks incomplete E class body)
- **Legacy:** No ErrorNode; `def method1` still at same indentation level

**Notable difference:** Prism uses ErrorNode to mark the position where the `Inner` class definition was incomplete

#### class_indent_6.rb
- **Prism:** ErrorNode at line 15 (marks failed Inner class in F)
- **Legacy:** No ErrorNode

### class_weird_newline_indent.rb (Most Complex Case)

This file tests class definitions across multiple nested scopes with unusual newlines.

**Structural differences:**

1. **Nesting scope:**
   - Prism keeps nested classes inside their parent's body (B, C, D, E, F become nested inside A)
   - Legacy flattens them as siblings to A

2. **ErrorNode:**
   - Prism: Single ErrorNode at line 50 (inside F's Inner class where parsing failed)
   - Legacy: No ErrorNode

3. **Superclass tracking:**
   - Prism sometimes uses `(<emptyTree>::<C Object>)` as explicit superclass
   - Legacy also uses this form in same places
   - No discrepancy in superclass representation

4. **Method/statement scoping:**
   - Legacy correctly associates methods with their classes at the proper indentation level
   - Prism includes statement bodies in more nested contexts before marking ErrorNode

### Summary of Patterns

| Aspect | Prism | Legacy | Status |
|--------|-------|--------|--------|
| ErrorNode presence | Present at failure points | Often absent | Clear diagnostic difference |
| $ variable numbering | Identical | Identical | No differences |
| Superclass syntax | Same format | Same format | No differences |
| Method placement | Correct association | Correct association | No differences |
| Statement scoping | Varies by error context | Cleaner scope attribution | Prism more literal about parse state |
| Nesting scope (weird_newline) | Preserves nesting hierarchy | Flattens to siblings | Significant structural difference |

## Implications

1. **ErrorNode is diagnostic:** The Prism parser includes ErrorNode to mark where recovery began, providing better error context for developers.

2. **Scope flattening:** The legacy parser flattens nested class scopes in error recovery scenarios, while Prism preserves the attempted nesting. This is a meaningful difference in error recovery strategy.

3. **Backward compatibility:** If tests are being updated for Prism, the ErrorNode additions and scope preservation changes represent legitimate improvements to error diagnostics, not bugs.

4. **No variable numbering impact:** Since variable numbering is identical, this difference doesn't affect symbol/constant tracking systems.
