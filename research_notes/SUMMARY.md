# Prism vs Legacy Parser: Research Notes Summary

## Variable Numbering (`$`) Differences

Only **3 files** have `$` variable numbering differences:

| File | Node Types Affected | Details | Verdict |
|------|-------------------|---------|---------|
| `assign_to_index_raw.md` | `assignTemp` | Prism uses fewer temps for block-pass cases ($89 max vs $99). Cascading offset from eliminating Magic constant, method name literal, and block temp assignments. | **Tolerable** (improvement) |
| `pattern_matching_hash.md` | `assignTemp` | `$4` vs `$3` for first temp variable in pattern match | **Tolerable** (implementation detail) |
| `misc_and_special.md` (misc.rb) | `assignTemp` | Prism generates extra `$13`, `$14` in nested destructuring where legacy stops at `$12` | **Bug** (over-generates nesting) |

All other files (15 of 18) have **no** `$` variable numbering differences.

---

## Other Categories of Differences

### 1. Error Recovery: Nesting Strategy

**Pattern:** When encountering unclosed methods/classes/modules, Prism nests subsequent statements inside the unclosed scope (often wrapping in `begin...end` blocks), while the legacy parser closes the unclosed scope and continues at the parent level. Prism also appends `::ErrorNode` markers.

**Files:** `class_indent.md` (7 pairs), `defn_indent.md` (6 pairs), `defs_indent.md` (6 pairs), `module_indent.md` (6 pairs), `eof.md` (8 pairs), `lonely_def.md` (13 pairs), `if_indent.md` (6 pairs), `remaining_error_recovery.md` (24 pairs), `lsp_completion.md` (case_1, case_2, missing_const_name, missing_fun)

**Verdict:** **Tolerable.** Both strategies are valid error recovery approaches. Prism's is arguably more faithful to the source structure.

### 2. Error Node Representation

**Pattern:** Prism uses `::ErrorNode` while legacy uses `<emptyTree>::<C <ErrorNode>>`.

**Files:** Nearly all error recovery files (`if_indent.md`, `remaining_error_recovery.md`, `lsp_completion.md`, etc.)

**Verdict:** **Tolerable.** Cosmetic difference in how error nodes are represented.

### 3. Duplicate Kwarg Numbering Missing

**Pattern:** Prism fails to add `$N` suffixes to duplicate keyword parameters. E.g., `def foo(x:, x:)` instead of `def foo(x:, x$1:)`.

**Files:** `kwarg_diffs.md` (`duplicate_kwarg.rb`, `fuzz_repeated_kwarg.rb`)

**Verdict:** **Bug.** Creates ambiguous parameter names. Systematic issue across all duplicate kwarg cases.

### 4. Block Parameter Naming (`_1` vs `x`)

**Pattern:** In numbered parameter contexts (`_1`, `_2`), Prism generates `|x|` in block signatures while legacy generates `|_1|`.

**Files:** `remaining_error_recovery.md` (`numparams.rb`, `numparams_crash_1.rb`, `numparams_crash_2.rb`, `numparams_crash_5.rb`)

**Verdict:** **Bug.** Prism should use `_1` to match the numbered parameter semantics.

### 5. Splat/Destructure `expand-splat` Argument + `to_ary` vs `[]`

**Pattern:** Prism uses `expand-splat(..., 0, 0)` with `.to_ary()`, while legacy uses `expand-splat(..., 1, 0)` with `.[](0)`. These are interdependent: different start positions require different extraction methods.

**Files:** `call_block_param_raw.md`, `star_in_block_arg.md`

**Verdict:** **Bug (likely).** The semantics differ -- `.to_ary()` returns the full array while `.[](0)` returns only the first element. Needs investigation of which is correct for each context.

### 6. Block Pass Desugaring Strategy

**Pattern:** Prism uses direct `[]=` calls for block-pass compound assignment. Legacy uses `Magic.call-with-block-pass()` with extra temps. For *invalid* block pass, Prism desugars with `Magic.call-with-block-pass`, while legacy uses `ErrorNode` in arg position.

**Files:** `assign_to_index_raw.md`, `bad_block_pass.md`

**Verdict:** **Tolerable.** Semantically equivalent, Prism's approach is simpler.

### 7. Pattern Matching Hash Desugaring

**Pattern:** Prism produces `ErrorNode` markers and raw method calls instead of proper variable assignments for hash pattern matching.

**Files:** `pattern_matching_hash.md`

**Verdict:** **Bug.** Variables `e`, `g`, `h`, `i` should be assigned from destructured pattern match, not left as ErrorNodes.

### 8. Proc Block Parameter Lost

**Pattern:** `<self>.proc() do |x|` becomes `<self>.proc() do ||` -- block parameter dropped.

**Files:** `misc_and_special.md` (misc.rb)

**Verdict:** **Bug.** Semantic information lost.

### 9. Error Node Extraction (LSP Completion)

**Pattern:** Prism extracts ErrorNodes as separate statements instead of embedding them in parent expressions (e.g., array literals, function arguments).

**Files:** `lsp_completion.md` (`bad_arguments.rb`, `bad_list_elems.rb`, `eof.rb`)

**Verdict:** **Bug.** ErrorNodes should remain inline in their parent expressions.

### 10. Line Number Off-by-One in Symbol Tables

**Pattern:** All location line numbers in Prism symbol tables are offset by +1.

**Files:** `destructure_symtab.md`, `misc_and_special.md` (sig_misc.rb)

**Verdict:** **Tolerable bug** (low severity). Consistent and predictable; purely affects diagnostics.

### 11. `<cast:let>` Wrapping in Heredoc Rewriter

**Pattern:** Prism wraps heredoc assignments in `<cast:let>()` nodes that legacy doesn't produce.

**Files:** `misc_and_special.md` (assertions_heredoc_modified.rb)

**Verdict:** **Tolerable.** Type annotation metadata; actual values unchanged.

### 12. Unexpected ErrorNode in minitest rewriter

**Pattern:** Prism output includes a trailing `::ErrorNode` that the standard version doesn't.

**Files:** `misc_and_special.md` (minitest_empty_test_each.rb)

**Verdict:** **Bug.** Indicates a real parsing issue with this input.

### 13. `disable-parser-comparison` Directive

**Pattern:** Prism autocorrects file includes an extra `# disable-parser-comparison: true` comment.

**Files:** `crash_block_pass_autocorrects.md`

**Verdict:** **Tolerable.** Test infrastructure choice, not a functional difference.

---

## Bugs to Fix (Action Items)

Ordered by severity:

1. **Duplicate kwarg numbering missing** (`kwarg_diffs.md`) -- Creates ambiguous parameter names. Prism must add `$N` suffixes to duplicate kwargs. Affects all functions with repeated parameter names.

2. **Pattern matching hash desugaring broken** (`pattern_matching_hash.md`) -- Variables not assigned from destructured hash patterns. Produces ErrorNodes instead of proper bindings.

3. **Block parameter lost in proc** (`misc_and_special.md`, misc.rb) -- `|x|` becomes `||`, losing semantic information.

4. **Numbered parameter naming** (`remaining_error_recovery.md`, numparams files) -- Prism uses `|x|` instead of `|_1|` for numbered block parameters.

5. **ErrorNode extraction from expressions** (`lsp_completion.md`, bad_arguments.rb, bad_list_elems.rb) -- ErrorNodes should stay inline in parent expressions, not become separate statements.

6. **Splat/destructure `expand-splat` semantics** (`call_block_param_raw.md`, `star_in_block_arg.md`) -- `to_ary()` vs `.[](0)` and position arg 0 vs 1. Needs investigation to determine which is correct.

7. **Extra nesting in destructuring** (`misc_and_special.md`, misc.rb) -- Prism generates unnecessary extra temps `$13`/`$14` for nested destructuring.

8. **Unexpected ErrorNode in minitest rewriter** (`misc_and_special.md`) -- Spurious error marker in Prism output.

---

## Tolerable Differences

- **Error recovery nesting strategy** -- Prism nests inside unclosed scopes; legacy flattens. Both valid. (class/defn/defs/module_indent, eof, lonely_def, if_indent, remaining_error_recovery, lsp_completion)
- **ErrorNode representation** -- `::ErrorNode` vs `<emptyTree>::<C <ErrorNode>>`. Cosmetic.
- **Block-pass desugaring** -- Prism uses fewer temps and simpler structure. Improvement.
- **Rescue clause splat representation** -- Comma-separated vs `.concat()` chaining. Semantically equivalent.
- **Line number off-by-one in symbol tables** -- Consistent +1 offset. Low severity.
- **`<cast:let>` wrapping in heredocs** -- Type annotation metadata only.
- **`disable-parser-comparison` directive** -- Test infrastructure.
- **Rational literal representation** -- `"5r"` vs `"5"`. Minor formatting.

---

## Statistics

| Metric | Count |
|--------|-------|
| Total research note files | 18 |
| Total test pairs analyzed (approx) | ~120+ |
| Files with `$` numbering differences | 3 |
| Bugs identified | 8 |
| Tolerable differences | 8 categories |
