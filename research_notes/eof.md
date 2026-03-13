# EOF Error Recovery Test Comparison

Comparing 8 pairs of desugar-tree output files:
- `.desugar-tree.exp` (original/non-Prism parser)
- `.desugar-tree.prism.exp` (Prism parser)

## Key Finding: `$` Variable Numbering

**Only eof_7 has a `$` variable and it's identical in both versions.**

| Test | Non-Prism | Prism | Match? |
|------|-----------|-------|--------|
| eof_1 | No `$` vars | No `$` vars | ✓ |
| eof_2 | No `$` vars | No `$` vars | ✓ |
| eof_3 | No `$` vars | No `$` vars | ✓ |
| eof_4 | No `$` vars | No `$` vars | ✓ |
| eof_5 | No `$` vars | No `$` vars | ✓ |
| eof_6 | No `$` vars | No `$` vars | ✓ |
| eof_7 | `<rescueTemp>$2` | `<rescueTemp>$2` | ✓ IDENTICAL |
| eof_8 | No `$` vars | No `$` vars | ✓ |

---

## Detailed Comparisons

### eof_1: Class with method, sig, and nested method

**Source:** Class A with method foo, sig, and method bar, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 10: `def foo<<todo method>>(x, &<blk>)`
  - Line 11: `<self>.puts(x)` (directly in method body, not wrapped)
  - Line 13-15: `<self>.sig()` block and `def bar` follow at class scope
  - Line 21: `<self>.puts("after")` at class scope

**Prism (.desugar-tree.prism.exp):**
- Line 9: `def foo<<todo method>>(x, &<blk>)`
  - Line 10: `begin` wraps the entire method body
  - Line 11: `<self>.puts(x)` (inside begin)
  - Line 12-14: `<self>.sig()` block follows (still inside begin)
  - Line 15-20: `def bar` nested (still inside begin)
  - Line 17: `<self>.puts("after")` (inside bar's begin)
  - Line 18: `::<ErrorNode>` (inside bar's begin)

**Key Diff:** Prism wraps method bodies in `begin...end`, causing nesting structure to change. Non-Prism has flatter structure.

---

### eof_2: Nested defs

**Source:** def foo > def bar > def qux, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 2-14: Three nested defs
- Line 3: `begin` block wraps defs starting from bar
- Line 7-8: puts statements for "inside qux" and "inside bar"
- Line 12: puts for "inside foo"

**Prism (.desugar-tree.prism.exp):**
- Line 2-13: Three nested defs
- Line 5: Single `begin` block containing ALL puts statements
- All puts statements (6-8) are collapsed into one begin block
- Line 9: `::<ErrorNode>` at end

**Key Diff:** Prism collapses all nested method bodies into a single begin block with all statements flat.

---

### eof_3: Nested modules with implicit constant

**Source:** module A > module B > module (anonymous) > C, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 5: `::<emptyTree>` inside module C

**Prism (.desugar-tree.prism.exp):**
- Line 5: `::<ErrorNode>` inside module C

**Key Diff:** Non-Prism ends with `<emptyTree>`, Prism with `::<ErrorNode>`.

---

### eof_4: Nested modules with complete structure

**Source:** module A > B > C (all complete with end), but missing outer class end.

**Non-Prism (.desugar-tree.exp):**
- Line 5: `::<emptyTree>` inside module C
- Modules fully scoped

**Prism (.desugar-tree.prism.exp):**
- Line 5: `::<emptyTree>` inside module C (same)
- Line 9: `::<ErrorNode>` after module B ends (outside C)

**Key Diff:** Prism adds `::<ErrorNode>` at line 9 after the properly-closed modules, Non-Prism doesn't have it.

---

### eof_5: Mixed class/module/def nesting

**Source:** class A > module B > def self.foo > def bar, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 4-11: def self.foo with begin block
- Line 6-8: def bar with empty body, then puts statements at module scope
- Line 13: `<self>.puts("inside A")` at class scope

**Prism (.desugar-tree.prism.exp):**
- Line 4-12: def self.foo with begin block
- Line 5-12: def bar nested inside begin block
- Line 7-8: Both puts statements inside bar's begin block
- Line 9: `::<ErrorNode>`

**Key Diff:** Prism nests def bar and puts statements inside self.foo's begin. Non-Prism has them at module scope.

---

### eof_6: Class with method containing unclosed if

**Source:** class A > def foo(x) > if x (unclosed), ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 3-9: def foo with if/else
- Line 5: `<self>.puts(x)` in if branch
- Line 7: else branch with `<emptyTree>`

**Prism (.desugar-tree.prism.exp):**
- Line 3-12: def foo with if/else
- Line 4: if condition
- Line 5-8: if branch with begin block
- Line 6: `<self>.puts(x)` (inside begin)
- Line 7: `::<ErrorNode>` (inside begin)
- Line 9-11: else branch with `<emptyTree>`

**Key Diff:** Prism wraps the if branch in begin and adds `::<ErrorNode>`. Non-Prism has direct puts and no error node.

---

### eof_7: Method with nested begins and rescue (HAS $2 VARIABLE)

**Source:** def foo(x) > puts > begin > puts > begin with rescue, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
```
10:        rescue => <rescueTemp>$2
11:          <self>.puts("inside rescue")
```

**Prism (.desugar-tree.prism.exp):**
```
10:        rescue => <rescueTemp>$2
11:          begin
12:            <self>.puts("inside rescue")
13:            ::<ErrorNode>
14:          end
```

**Key Diff:** The `<rescueTemp>$2` variable numbering is **IDENTICAL** in both. Prism wraps the rescue clause body in a begin block and adds `::<ErrorNode>`.

---

### eof_8: Method with multiple control structures (unless, while, until, do, lambda, do)

**Source:** def foo(x, y) > unless/while/until/do/lambda/do blocks, ending at EOF.

**Non-Prism (.desugar-tree.exp):**
- Line 4-19: Deeply nested if/else > while > while > times.do > lambda.do > x.y.do
- Line 12: `::<emptyTree>` at deepest level (inside x.y.do)

**Prism (.desugar-tree.prism.exp):**
- Line 4-19: Same nesting structure (unless becomes if x, while x.!() becomes second while)
- Line 12: `::<ErrorNode>` at deepest level (inside x.y.do)

**Key Diff:** Non-Prism ends with `::<emptyTree>`, Prism with `::<ErrorNode>` at the deepest block.

---

## Summary of Differences

### Structure Differences
1. **begin blocks**: Prism wraps method bodies in explicit `begin...end` constructs. Non-Prism leaves them implicit.
2. **Error recovery**: Prism consistently terminates incomplete blocks with `::<ErrorNode>`. Non-Prism uses `::<emptyTree>` or nothing.
3. **Nesting depth**: Prism tends to nest statements deeper into begin blocks than Non-Prism.

### Variable Numbering
- **NO DIFFERENCES in $ variable numbering**
- Only test with $ variables: eof_7, which has `<rescueTemp>$2` in both versions
- This suggests variable numbering is stable across both parsers

### Error Node Handling
- **eof_3**: Non-Prism `<emptyTree>` → Prism `::<ErrorNode>`
- **eof_4**: Non-Prism (no error node) → Prism `::<ErrorNode>`
- **eof_6**: Non-Prism (no error node) → Prism `::<ErrorNode>`
- **eof_8**: Non-Prism `::<emptyTree>` → Prism `::<ErrorNode>`

### Tests with Matching Structure (besides variable numbering)
- eof_2: Both collapse nested method bodies into a single begin block structure
- eof_7: Rescue variable naming is identical

