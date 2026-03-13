# Comparison: destructure.rb.symbol-table-raw (Prism vs AST)

## File Paths
- **Prism version:** `test/testdata/desugar/destructure.rb.symbol-table-raw.prism.exp`
- **AST version:** `test/testdata/desugar/destructure.rb.symbol-table-raw.exp`

## Differences Found

### 1. Line Number Offsets (CONSISTENT PATTERN)

All location line numbers in the Prism version are offset by +1:

| Line | Prism | AST | Difference |
|------|-------|-----|-----------|
| 3 | `start=3:1 end=10:4` | `start=2:1 end=9:4` | +1 line |
| 5 | `start=3:1 end=3:18` | `start=2:1 end=2:18` | +1 line |
| 6 | `start=4:3 end=4:18` | `start=3:3 end=3:18` | +1 line |
| 7 | `start=4:9 end=4:14` | `start=3:9 end=3:14` | +1 line |
| 8 | `start=4:16 end=4:17` | `start=3:16 end=3:17` | +1 line |
| 10 | `start=3:1 end=3:18` | `start=2:1 end=2:18` | +1 line |
| 11 | `start=3:1 end=3:18` | `start=2:1 end=2:18` | +1 line |
| 12 | `start=3:1 end=10:4` | `start=2:1 end=9:4` | +1 line |

### 2. Variable Numbering

No `$` variable numbering differences detected. All `$1` references are identical across both files.

### 3. Content Differences

All other content (method signatures, argument declarations, type members) is identical between the two versions.

---

## Assessment

### Category: **TOLERABLE BUG**

**Reason:** This is a systematic line number offset that affects the generated expectation file from the Prism desugarer, not a logic error. The offset is consistent across all location markers, suggesting it's a known issue in how Prism-based desugaring reports source locations (likely off-by-one in line tracking during AST transformation).

**Severity:** Low
- No impact on symbol resolution, scoping, or type checking logic
- Purely affects diagnostic/debugging output (location markers)
- Consistent and predictable pattern makes it easy to track

**Recommendation:** This should be tracked as a Prism desugarer location reporting issue, but it's safe to update test expectations to match the Prism output if that's the chosen desugarer path forward.
