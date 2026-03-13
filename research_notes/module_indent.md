# Module Indent Test Comparison: .exp vs .prism.exp

## Summary

All 6 pairs of test files (module_indent_1 through module_indent_6) show a consistent pattern:
- **No `$` variable numbering differences found** (neither format uses `$` variables)
- The key difference is structural: **Prism desugaring moves definitions that belong in modules INTO the module scope**, while the original desugarer places them outside.
- Each .prism.exp file adds an `ErrorNode` at the end of the class (but outside the inner module), representing error recovery.

## Detailed Comparison

### Test 1 (module_indent_1)

**Source:** Empty module with method2 defined outside
```ruby
class A
  module Inner
  # missing 'end'
  def method2
  end
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| method2 location | Inside A, outside Inner | Inside Inner |
| Error recovery | None | `ErrorNode` at class level |

**Key difference:** In .prism.exp, `method2` is nested inside `Inner`, where it syntactically belongs.

---

### Test 2 (module_indent_2)

**Source:** Module with two puts, then method2 defined outside
```ruby
class B
  module Inner
    puts 'hello'
    puts 'hello'
  # missing 'end'
  def method2
  end
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| puts x2 location | Inside Inner | Inside Inner |
| method2 location | Inside B, outside Inner | Inside Inner (after puts) |
| Error recovery | None | `ErrorNode` at class level |

---

### Test 3 (module_indent_3)

**Source:** Module with sig/void, then method2 defined outside
```ruby
class C
  module Inner
  # missing 'end'
  sig {void}
  def method2
  end
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| sig/void location | Inside B, outside Inner | Inside Inner |
| method2 location | Inside B, outside Inner | Inside Inner |
| Error recovery | None | `ErrorNode` at class level |

---

### Test 4 (module_indent_4)

**Source:** Module with puts, sig/void, method2 defined outside
```ruby
class D
  module Inner
    puts 'hello'
  # missing 'end'
  sig {void}
  def method2
  end
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| puts location | Inside Inner | Inside Inner |
| sig/void location | Inside D, outside Inner | Inside Inner (after puts) |
| method2 location | Inside D, outside Inner | Inside Inner (after sig) |
| Error recovery | None | `ErrorNode` at class level |

---

### Test 5 (module_indent_5)

**Source:** Sig/method1 inside C, then module Inner (no content)
```ruby
class E
  sig {void}
  def method1
  end

  module Inner
  # missing 'end'
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| sig/method1 location | Inside E | Inside E |
| Inner location | Inside E | Inside E |
| Inner content | Empty | Empty |
| Error recovery | None | `ErrorNode` at class level |

**Note:** In test 5, both parsers agree on the structure (unlike tests 1-4) because the method is defined before the module.

---

### Test 6 (module_indent_6)

**Source:** Sig/method1 inside F, then module Inner with puts
```ruby
class F
  sig {void}
  def method1
  end

  module Inner
    puts 'hello'
  # missing 'end'
end
```

| Aspect | .exp (Original) | .prism.exp (Prism) |
|--------|-----------------|-------------------|
| sig/method1 location | Inside F | Inside F |
| Inner location | Inside F | Inside F |
| puts location | Inside Inner | Inside Inner |
| Error recovery | None | `ErrorNode` at class level |

**Note:** Similar to test 5 - both agree because method is defined before the module.

---

## Pattern Analysis

### $ Variable Numbering
**No differences found.** Neither the original .exp nor .prism.exp files use `$N` temporary variables in these tests.

### Structural Differences

The core difference between the two desugaring approaches:

1. **Original desugarer (.exp):** When a module is unclosed (missing `end`), content that follows is placed at the parent class level, outside the module scope.

2. **Prism desugarer (.prism.exp):**
   - Attributes content to the module scope based on indentation heuristics
   - Moves method definitions and other code inside the module where they syntactically belong
   - Adds an `ErrorNode` to the parent class for error recovery

### When Differences Appear

- **Tests 1-4:** Differences occur because method2 (and sig blocks) are indented to appear inside Inner but are syntactically at the class level due to the missing module `end`.
- **Tests 5-6:** No structural differences because methods are defined before the module, so there's no ambiguity.

## Conclusion

The Prism desugarer uses indentation-based heuristics to recover from the missing module `end` by placing code where it indentation-wise belongs, rather than at the parent scope. This is a more intuitive error recovery strategy that maintains the intended nesting structure.
