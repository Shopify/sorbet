# defs_indent Test Comparison: .prism.exp vs .exp

## Summary
Comparing 6 test pairs for desugar-tree output under Prism vs legacy parser, focusing on indentation error recovery. All files involve incomplete method definitions with indentation errors.

Key finding: **Prism parser inserts additional error recovery structures (nesting methods within parent methods, wrapping in `begin` blocks, and appending `::<ErrorNode>`) that the legacy parser does not.**

---

## Test 1: defs_indent_1.rb

**Input:**
```ruby
class A
  def self.method1
  # missing body and end

  def self.method2
  end
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      def self.method2<<todo method>>(&<blk>)    # method2 nested INSIDE method1
        <emptyTree>
      end
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)      # method2 at same level as method1
      <emptyTree>
    end
  end
end
```

**Differences:**
- Prism: Nests method2 inside method1's body
- Prism: Adds `::<ErrorNode>` at class body level
- Legacy: Both methods at same nesting level, properly separated

---

## Test 2: defs_indent_2.rb

**Input:**
```ruby
class B
  def self.method1(abc, xyz)
  # missing end
    puts 'hello'
    puts 'hello'

  def self.method2
  end
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
        def self.method2<<todo method>>(&<blk>)  # method2 nested inside begin
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
      end
    end

    def self.method2<<todo method>>(&<blk>)      # method2 at class level
      <emptyTree>
    end
  end
end
```

**Differences:**
- Prism: Wraps puts statements in `begin..end` AND nests method2 inside that begin block
- Prism: Adds `::<ErrorNode>` at class level
- Legacy: Begin block only wraps puts statements; method2 separate at class level

---

## Test 3: defs_indent_3.rb

**Input:**
```ruby
class C
  def self.method1
  # missing end

  sig {void}
  def self.method2
  end
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      begin
        <self>.sig() do ||
          <self>.void()
        end
        def self.method2<<todo method>>(&<blk>)  # method2 nested inside begin
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||                            # sig at class level
      <self>.void()
    end

    def self.method2<<todo method>>(&<blk>)      # method2 at class level
      <emptyTree>
    end
  end
end
```

**Differences:**
- Prism: Wraps sig call in `begin..end` inside method1; nests method2 inside begin block
- Prism: Adds `::<ErrorNode>`
- Legacy: sig and method2 both at class level; method1 has empty body

---

## Test 4: defs_indent_4.rb

**Input:**
```ruby
class D
  def self.method1
  # missing end
    puts 'hello'

  sig {void}
  def self.method2
  end
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      begin
        <self>.puts("hello")
        <self>.sig() do ||
          <self>.void()
        end
        def self.method2<<todo method>>(&<blk>)  # method2 nested inside begin
          <emptyTree>
        end
      end
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    <self>.sig() do ||                            # sig at class level
      <self>.void()
    end

    def self.method2<<todo method>>(&<blk>)      # method2 at class level
      <emptyTree>
    end
  end
end
```

**Differences:**
- Prism: Wraps puts + sig + method2 in `begin..end` inside method1
- Prism: Adds `::<ErrorNode>`
- Legacy: puts inside method1; sig and method2 at class level

---

## Test 5: defs_indent_5.rb

**Input:**
```ruby
class E
  sig {void}
  def self.method1
  end

  def self.method2
  # missing end
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

**Differences:**
- Prism: Adds `::<ErrorNode>` at class level (only difference)
- Both properly parse sig, method1, and method2 at class level
- Minimal difference; error is at EOF

---

## Test 6: defs_indent_6.rb

**Input:**
```ruby
class F
  sig {void}
  def self.method1
  end

  def self.method2
  # missing end
    puts 'hello'
end
```

### Prism (.prism.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    ::<ErrorNode>                                 # Error recovery node
  end
end
```

### Legacy (.exp)
```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end
  end
end
```

**Differences:**
- Prism: Adds `::<ErrorNode>` at class level (only difference)
- Both properly parse all statements
- Minimal difference; error is at EOF

---

## Pattern Analysis

### $ Variable Numbering
No `$` variables appear in any of these test outputs. The comparison does not reveal $ variable numbering differences.

### Structural Differences Across All 6 Tests

| Test | Key Prism Behavior | Legacy Behavior |
|------|-------------------|-----------------|
| 1 | Nests method2 inside method1; adds ::<ErrorNode> | Methods at same level |
| 2 | Nests method2 in begin block; adds ::<ErrorNode> | begin wraps only puts; method2 separate |
| 3 | Wraps sig+method2 in begin inside method1; adds ::<ErrorNode> | sig and method2 at class level |
| 4 | Wraps puts+sig+method2 in begin; adds ::<ErrorNode> | puts in method1; sig+method2 at class level |
| 5 | Adds ::<ErrorNode> only | Identical structure |
| 6 | Adds ::<ErrorNode> only | Identical structure |

### Core Insights
1. **Prism's aggressive nesting**: When method1 is unclosed, Prism pulls subsequent statements (method2, sig calls, puts) into method1's body, often wrapping them in `begin..end` blocks.

2. **Error marker**: Prism consistently appends `::<ErrorNode>` at the class body level to indicate error recovery occurred. Legacy parser does not.

3. **Scope preservation**: Legacy parser maintains scope isolation better—statements intended for class level stay there even with unclosed methods.

4. **Begin block generation**: Prism wraps accumulated statements in `begin..end`, while legacy parser doesn't introduce artificial control structures.
