# crash_block_pass_suggestion.rb.autocorrects Comparison

## File Paths
- Prism version: `test/testdata/parser/crash_block_pass_suggestion.rb.autocorrects.prism.exp`
- Standard version: `test/testdata/parser/crash_block_pass_suggestion.rb.autocorrects.exp`

## Differences

| Line | Prism (.prism.exp) | Standard (.exp) | Status |
|------|-------------------|-----------------|--------|
| 3 | `# disable-parser-comparison: true` | (absent) | Added in Prism |

## Analysis

### Difference Detail
The Prism version includes an additional comment directive on line 3:
```
# disable-parser-comparison: true
```

This directive is absent from the standard expectation file.

### Categorization: **Tolerable**

**Reasoning:**
- The `disable-parser-comparison: true` directive is a test configuration directive, not a functional difference in the error message or autocorrect output
- Both files contain identical error messaging: `block pass should not be enclosed in curly braces`
- Both files have the same AST structure and class/method definition
- This is likely added intentionally in the Prism version to disable comparison during development/testing, possibly because Prism and the standard parser are still converging on behavior
- The presence of this directive does not indicate a bug or regression—it's a test infrastructure choice

### Conclusion
This is a tolerable difference. The actual parser output and error detection are identical between the two versions. The only difference is a test directive controlling whether parser comparison validation should run.
