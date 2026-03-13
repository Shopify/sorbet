# defn_indent Test Cases: Prism vs Standard Parser Comparison

## Overview
Comparing `.desugar-tree.prism.exp` (Prism desugarer output) vs `.desugar-tree.exp` (standard parser output) for all 6 test cases.

## Key Finding: `ErrorNode` Handling

The most significant difference across all cases is how the parsers handle error recovery for unclosed method definitions with indentation errors.

**Standard Parser**: Recovers gracefully and continues parsing subsequent methods at class level.

**Prism Parser**: Generates an `ErrorNode` at class level after the unclosed method definition, creating structural differences.

---

## Case-by-Case Analysis

### Case 1 (defn_indent_1)
**Source**: Unclosed `method1`, then properly closed `method2`

**Standard (.exp)**:
```
    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
```

**Prism (.prism.exp)**:
```
    def method1<<todo method>>(&<blk>)
      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    ::<ErrorNode>
```

**Difference**: Prism nests `method2` inside `method1` and adds `ErrorNode` at class level. Standard parser treats both as sibling methods at class level.

---

### Case 2 (defn_indent_2)
**Source**: `method1` with body statements, unclosed, then `method2`

**Standard (.exp)**:
```
    def method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
      end
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
```

**Prism (.prism.exp)**:
```
    def method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
        def method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>
```

**Difference**: Prism wraps the statements in a `begin`/`end` block and nests `method2` inside it. Standard parser has the `begin`/`end` at method body level only, without nesting `method2`.

---

### Case 3 (defn_indent_3)
**Source**: `method1` unclosed, `sig {void}` annotation, then `method2`

**Standard (.exp)**:
```
    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
```

**Prism (.prism.exp)**:
```
    def method1<<todo method>>(&<blk>)
      begin
        <self>.sig() do ||
          <self>.void()
        end
        def method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>
```

**Difference**: Prism wraps both the `sig` call and `method2` inside `method1` in a `begin`/`end`. Standard parser treats all three as separate, sibling statements at class level.

---

### Case 4 (defn_indent_4)
**Source**: `method1` with `puts`, unclosed, `sig {void}`, then `method2`

**Standard (.exp)**:
```
    def method1<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
```

**Prism (.prism.exp)**:
```
    def method1<<todo method>>(&<blk>)
      begin
        <self>.puts("hello")
        <self>.sig() do ||
          <self>.void()
        end
        def method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>
```

**Difference**: Prism wraps the `puts`, `sig`, and `method2` inside `method1` in a `begin`/`end`. Standard parser treats the `puts` as `method1`'s body, then has `sig` and `method2` as separate class-level statements.

---

### Case 5 (defn_indent_5)
**Source**: Properly closed `method1` after `sig {void}`, then unclosed `method2`

**Standard (.exp)**:
```
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
```

**Prism (.prism.exp)**:
```
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end

    ::<ErrorNode>
```

**Difference**: Both are identical structurally! Prism only adds `ErrorNode` at the end. This is because the error occurs at EOF (unclosed `method2`), not in the middle of the class body.

---

### Case 6 (defn_indent_6)
**Source**: Properly closed `method1` after `sig {void}`, then unclosed `method2` with body

**Standard (.exp)**:
```
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end
```

**Prism (.prism.exp)**:
```
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    ::<ErrorNode>
```

**Difference**: Identical structurally, both include the `puts("hello")` in `method2`. Prism adds `ErrorNode` at EOF only.

---

## Variable Numbering Differences

**NO DOLLAR-SIGN VARIABLE NUMBERING FOUND** in any of these test cases. Both parsers use the same placeholder syntax (`<blk>`, `<self>`, `<emptyTree>`, etc.) without numeric variable suffixes.

---

## Summary of Structural Differences

| Case | Structural Issue | Prism Behavior | Standard Behavior |
|------|-----------------|-----------------|-------------------|
| 1 | Unclosed method at start | Nests next method inside unclosed one, adds ErrorNode | Treats both as siblings, closes first method |
| 2 | Unclosed method with body | Wraps all following statements in begin/end inside unclosed method | Separates method body from following methods |
| 3 | Unclosed method, sig annotation | Nests sig and next method inside unclosed method in begin/end | Treats all as siblings |
| 4 | Unclosed method with body, sig | Nests all following in begin/end inside unclosed method | Treats method body separately from sig/next method |
| 5 | Properly closed methods, error at EOF | Identical to standard parser, adds ErrorNode | No ErrorNode |
| 6 | Properly closed methods, error at EOF | Identical to standard parser, adds ErrorNode | No ErrorNode |

The core difference: **Prism's error recovery for mid-class unclosed methods creates nested structures and wraps statements in implicit `begin`/`end` blocks, while the standard parser recovers by closing the unclosed method cleanly and continuing at class level.**
