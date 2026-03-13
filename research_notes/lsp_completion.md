# LSP Completion File Comparison: .prism.exp vs .exp

## Summary
Compared 8 file pairs from `test/testdata/lsp/completion/`. The Prism version (`.prism.exp`) shows significantly more verbose AST output with nested method definitions and intermediate error nodes that are collapsed or simplified in the standard version (`.exp`).

No `$` variable numbering differences found across any files.

---

## Detailed Findings

### 1. bad_arguments.rb

**Node Structure Differences:**
- `.prism.exp`: Flattens error recovery—shows many `::<ErrorNode>` nodes interspersed with actual expressions
- `.exp`: Clean, minimal structure with actual method calls and no intermediate error nodes

**Example:**
```
Prism: <self>.foo(a, :x, ::<ErrorNode>)
       ::<ErrorNode>
       y
       ::<ErrorNode>
       <self>.foo(a, :x, x)

Exp:   <self>.foo(a, <emptyTree>::<C <ErrorNode>>, :y, y)
       <self>.foo(a, :x, <emptyTree>::<C <ErrorNode>>, :y, y)
```

**Category:** BUG - Prism desugarer is outputting error nodes as separate statements rather than embedding them in expressions.

---

### 2. bad_list_elems.rb

**Node Structure Differences:**
- `.prism.exp`: Interleaves error nodes and expressions at statement level
- `.exp`: Wraps error nodes in array literals with actual content

**Example:**
```
Prism: [::<ErrorNode>]
       ::<ErrorNode>
       x
       ::<ErrorNode>
       [x, y]

Exp:   [<emptyTree>::<C <ErrorNode>>, x]
       [<emptyTree>::<C <ErrorNode>>]
       [x, <emptyTree>::<C <ErrorNode>>, y]
       [x, y, <emptyTree>::<C <ErrorNode>>]
```

**Category:** BUG - Prism is extracting error nodes as separate statements instead of keeping them as array elements.

---

### 3. case_1.rb

**Key Differences:**
- `.prism.exp`: Nests all 4 test methods inside `test1` method definition
- `.exp`: Defines 4 separate top-level methods

**Structure:**
```
Prism: def self.test1
         def self.test2
           def self.test3
             def self.test4
               # nested 3 levels deep
             end
           end
         end
       end

Exp:   def self.test1 ... end
       def self.test2 ... end
       def self.test3 ... end
       def self.test4 ... end
```

**Variable Numbers:** All use `<assignTemp>$2` consistently (no difference).

**Category:** TOLERABLE - This is likely due to Prism's error recovery strategy keeping incomplete code nested rather than promoting definitions to parent scope. The desugared code still executes semantically.

---

### 4. case_2.rb

**Key Differences:**
- `.prism.exp`: Same nesting behavior as case_1.rb—methods 2-4 nested inside test1
- `.exp`: Separate top-level methods

**Structure:** Identical pattern to case_1.

**Variable Numbers:** All use `<assignTemp>$2` (no difference).

**Category:** TOLERABLE - Same as case_1.rb.

---

### 5. eof.rb

**Node Structure Differences:**
- `.prism.exp`: Has explicit `::<ErrorNode>` after `x.()` call
- `.exp`: Shows `x.<method-name-missing>()` instead

**Example:**
```
Prism: x.()
       ::<ErrorNode>

Exp:   x.<method-name-missing>()
```

**Category:** BUG - Error recovery differs. Prism leaves an orphaned error node; `.exp` provides more information about what was missing (method name).

---

### 6. missing_assign_rhs.rb

**Node Structure Differences:**
- `.prism.exp`: Uses `::<ErrorNode>` as RHS
- `.exp`: Uses `<emptyTree>::<C <ErrorNode>>` as RHS

**Example:**
```
Prism: y = ::<ErrorNode>

Exp:   y = <emptyTree>::<C <ErrorNode>>
```

**Category:** TOLERABLE - Both represent the same semantic error (missing RHS). The difference is in representation style for the missing node. The Prism version is less structured but semantically equivalent.

---

### 7. missing_const_name.rb

**Key Differences:**
- `.prism.exp`: Nests methods inside test_constant_completion_with_no_name
- `.exp`: Separate top-level methods with `<ConstantNameMissing>` node

**Structure:**
```
Prism: def test_constant_completion_with_no_name
         def test_constant_completion_adjacent_missing_names
           def test_constant_completion_before_method
             def test_constant_completion_before_keyword
             end
           end
         end
         def test_constant_completion_before_variable ... end
       end

Exp:   def test_constant_completion_with_no_name ... end
       def test_constant_completion_adjacent_missing_names ... end
       def test_constant_completion_before_method ... end
       def test_constant_completion_before_keyword ... end
       def test_constant_completion_before_variable ... end
```

**Node Representation:**
- Prism also shows `<emptyTree>::<C <ConstantNameMissing>>` vs `.exp` uses the same
- Both reference error-handling nodes similarly

**Category:** TOLERABLE - Nesting behavior (as seen in case_1/case_2), but both files correctly represent missing constant names.

---

### 8. missing_fun.rb

**Key Differences:**
- `.prism.exp`: Extensive nesting—methods 2-8 nested inside TestClass1.test_method_in_class
- `.exp`: Separate method definitions with clear structure

**Structure:** Similar multi-level nesting pattern as seen in case_1/case_2/missing_const_name.

**Variable Numbers:** Uses `<assignTemp>$2` consistently (no difference).

**Category:** TOLERABLE - Same nesting behavior, but the desugared code is semantically valid. Error representation is consistent.

---

## Variable Numbering Analysis

**Finding:** NO `$` variable numbering differences across any file.

- All files consistently use `<assignTemp>$2` in both versions
- No AST node types show different numbering schemes
- Variable allocation is identical between Prism and standard desugaring

---

## Categorization Summary

| File | Issue Type | Category |
|------|-----------|----------|
| bad_arguments.rb | Error node extraction | BUG |
| bad_list_elems.rb | Error node extraction | BUG |
| case_1.rb | Method nesting | TOLERABLE |
| case_2.rb | Method nesting | TOLERABLE |
| eof.rb | Error node representation | BUG |
| missing_assign_rhs.rb | Error node style | TOLERABLE |
| missing_const_name.rb | Method nesting | TOLERABLE |
| missing_fun.rb | Method nesting | TOLERABLE |

---

## Root Causes

### Bugs (3 files):
1. **Error node extraction (bad_arguments, bad_list_elems)**: Prism desugarer is outputting orphaned error nodes as separate statements instead of embedding them in parent expressions. This appears to be an issue with how Prism's error recovery interacts with the desugaring logic.

2. **Error node representation (eof.rb)**: Inconsistent handling of missing method calls—Prism leaves an orphaned `::<ErrorNode>` while the standard version provides `<method-name-missing>` metadata.

### Tolerable Issues (5 files):
- **Method nesting** (case_1, case_2, missing_const_name, missing_fun): Prism's error recovery keeps incomplete method definitions nested at their parse location, while the standard parser promotes them. Both are semantically valid for completion/error recovery purposes.
- **Error node style** (missing_assign_rhs): Difference in how missing nodes are wrapped (`::` vs `<emptyTree>::<C>`), but semantically equivalent.
