# Error recovery comparison: Prism vs original

## assign.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
# Should still see at least method def (not body)
def test_bad_assign(x)
  x =
end # error: unexpected token

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/assign.rb:5: unexpected token "end" https://srb.help/2001
     5 |end # error: unexpected token
        ^^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/assign.rb:4: expected an expression after `=` https://srb.help/2001
     4 |  x =
            ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism provides a more accurate error location pointing directly at the incomplete assignment operator and a clearer message about what's missing.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_bad_assign<<todo method>>(x, &<blk>)
    x = <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_bad_assign<<todo method>>(x, &<blk>)
    x = <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## begin_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    begin # error: Hint: this "begin" token might not be properly closed
      puts 'inside'
    rescue
    puts 'after'
  end

  def test2
      begin # error: Hint: this "begin" token might not be properly closed
        puts 'inside'
    rescue
    puts 'after'
  end

  def test3
    begin # error: Hint: this "begin" token might not be properly closed
      puts 'inside'
    rescue
      puts 'after'
  end

  def test4
    begin # error: Hint: this "begin" token might not be properly closed
      puts 'inside'
    rescue
    puts 'after'
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/begin_1.rb:31: unexpected token "end of file" https://srb.help/2001
    31 |end # error: unexpected token "end of file"
    32 |

test/testdata/parser/error_recovery/begin_1.rb:5: Hint: this "begin" token might not be properly closed https://srb.help/2003
     5 |    begin # error: Hint: this "begin" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/begin_1.rb:9: Matching `end` found here but is not indented as far
     9 |  end
          ^^^

test/testdata/parser/error_recovery/begin_1.rb:12: Hint: this "begin" token might not be properly closed https://srb.help/2003
    12 |      begin # error: Hint: this "begin" token might not be properly closed
              ^^^^^
    test/testdata/parser/error_recovery/begin_1.rb:16: Matching `end` found here but is not indented as far
    16 |  end
          ^^^

test/testdata/parser/error_recovery/begin_1.rb:19: Hint: this "begin" token might not be properly closed https://srb.help/2003
    19 |    begin # error: Hint: this "begin" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/begin_1.rb:23: Matching `end` found here but is not indented as far
    23 |  end
          ^^^

test/testdata/parser/error_recovery/begin_1.rb:26: Hint: this "begin" token might not be properly closed https://srb.help/2003
    26 |    begin # error: Hint: this "begin" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/begin_1.rb:30: Matching `end` found here but is not indented as far
    30 |  end
          ^^^
Errors: 5
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/begin_1.rb:32: expected an `end` to close the `def` statement https://srb.help/2001
    32 |
        ^

test/testdata/parser/error_recovery/begin_1.rb:32: expected an `end` to close the `def` statement https://srb.help/2001
    32 |
        ^

test/testdata/parser/error_recovery/begin_1.rb:32: expected an `end` to close the `def` statement https://srb.help/2001
    32 |
        ^

test/testdata/parser/error_recovery/begin_1.rb:32: expected an `end` to close the `class` statement https://srb.help/2001
    32 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the root cause (improperly closed begin blocks due to indentation) and only reports generic missing end errors at EOF, while Original correctly identifies all four begin blocks with indentation problems.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      <self>.puts("inside")
    rescue => <rescueTemp>$2
      <self>.puts("after")
    end

    def test2<<todo method>>(&<blk>)
      <self>.puts("inside")
    rescue => <rescueTemp>$2
      <self>.puts("after")
    end

    def test3<<todo method>>(&<blk>)
      <self>.puts("inside")
    rescue => <rescueTemp>$2
      <self>.puts("after")
    end

    def test4<<todo method>>(&<blk>)
      <self>.puts("inside")
    rescue => <rescueTemp>$2
      <self>.puts("after")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("inside")
      rescue => <rescueTemp>$2
        <self>.puts("after")
        def test2<<todo method>>(&<blk>)
          begin
            <self>.puts("inside")
          rescue => <rescueTemp>$2
            <self>.puts("after")
            def test3<<todo method>>(&<blk>)
              begin
                <self>.puts("inside")
              rescue => <rescueTemp>$2
                <self>.puts("after")
                def test4<<todo method>>(&<blk>)
                  <self>.puts("inside")
                rescue => <rescueTemp>$2
                  <self>.puts("after")
                end
                <emptyTree>::<C <ErrorNode>>
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,27 +1,31 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def test1<<todo method>>(&<blk>)
-      <self>.puts("inside")
-    rescue => <rescueTemp>$2
-      <self>.puts("after")
+      begin
+        <self>.puts("inside")
+      rescue => <rescueTemp>$2
+        <self>.puts("after")
+        def test2<<todo method>>(&<blk>)
+          begin
+            <self>.puts("inside")
+          rescue => <rescueTemp>$2
+            <self>.puts("after")
+            def test3<<todo method>>(&<blk>)
+              begin
+                <self>.puts("inside")
+              rescue => <rescueTemp>$2
+                <self>.puts("after")
+                def test4<<todo method>>(&<blk>)
+                  <self>.puts("inside")
+                rescue => <rescueTemp>$2
+                  <self>.puts("after")
+                end
+                <emptyTree>::<C <ErrorNode>>
+              end
+            end
+          end
+        end
+      end
     end
-
-    def test2<<todo method>>(&<blk>)
-      <self>.puts("inside")
-    rescue => <rescueTemp>$2
-      <self>.puts("after")
-    end
-
-    def test3<<todo method>>(&<blk>)
-      <self>.puts("inside")
-    rescue => <rescueTemp>$2
-      <self>.puts("after")
-    end
-
-    def test4<<todo method>>(&<blk>)
-      <self>.puts("inside")
-    rescue => <rescueTemp>$2
-      <self>.puts("after")
-    end
   end
 end
```

</details>

## block_arg_and_block.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

[1,2,3].each do |x|
  foo(&bar) do puts(x) end
#     ^^^^  error: both block argument and literal block are passed
#      ^^^  error: Method `bar` does not exist
# ^^^       error: Method `foo` does not exist
end

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/block_arg_and_block.rb:4: both block argument and literal block are passed https://srb.help/2001
     4 |  foo(&bar) do puts(x) end
              ^^^^

test/testdata/parser/error_recovery/block_arg_and_block.rb:4: Method `bar` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  foo(&bar) do puts(x) end
               ^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/block_arg_and_block.rb:3:
     3 |[1,2,3].each do |x|
        ^

test/testdata/parser/error_recovery/block_arg_and_block.rb:4: Method `foo` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  foo(&bar) do puts(x) end
          ^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/block_arg_and_block.rb:3:
     3 |[1,2,3].each do |x|
        ^
Errors: 3
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/block_arg_and_block.rb:4: both block arg and actual block given; only one block is allowed https://srb.help/2001
     4 |  foo(&bar) do puts(x) end
                    ^^^^^^^^^^^^^^

test/testdata/parser/error_recovery/block_arg_and_block.rb:4: Method `bar` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  foo(&bar) do puts(x) end
               ^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/block_arg_and_block.rb:3:
     3 |[1,2,3].each do |x|
        ^

test/testdata/parser/error_recovery/block_arg_and_block.rb:4: Method `foo` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  foo(&bar) do puts(x) end
          ^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/block_arg_and_block.rb:3:
     3 |[1,2,3].each do |x|
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all three errors correctly but the first error's location points to the entire block instead of the block argument, making it slightly less precise than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do |x|
    ::<Magic>.<call-with-block-pass>(<self>, :foo, <self>.bar()) do ||
      <self>.puts(x)
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do |x|
    ::<Magic>.<call-with-block-pass>(<self>, :foo, <self>.bar()) do ||
      <self>.puts(x)
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## block_do_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    puts 'before'
    x.f do # error: Hint: this "do" token might not be properly closed
    puts 'after'
  end

  def test2
    puts 'before'
    x.f() do # error: Hint: this "do" token might not be properly closed
    puts 'after'
  end

  def test3
    puts 'before'
    f() do # error: Hint: this "do" token might not be properly closed
    puts 'after'
  end

  def test4
    puts 'before'
    f() do # error: Hint: this "do" token might not be properly closed
    puts 'after'
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/block_do_1.rb:27: unexpected token "end of file" https://srb.help/2001
    27 |end # error: unexpected token "end of file"
    28 |

test/testdata/parser/error_recovery/block_do_1.rb:6: Hint: this "do" token might not be properly closed https://srb.help/2003
     6 |    x.f do # error: Hint: this "do" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/block_do_1.rb:8: Matching `end` found here but is not indented as far
     8 |  end
          ^^^

test/testdata/parser/error_recovery/block_do_1.rb:12: Hint: this "do" token might not be properly closed https://srb.help/2003
    12 |    x.f() do # error: Hint: this "do" token might not be properly closed
                  ^^
    test/testdata/parser/error_recovery/block_do_1.rb:14: Matching `end` found here but is not indented as far
    14 |  end
          ^^^

test/testdata/parser/error_recovery/block_do_1.rb:18: Hint: this "do" token might not be properly closed https://srb.help/2003
    18 |    f() do # error: Hint: this "do" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/block_do_1.rb:20: Matching `end` found here but is not indented as far
    20 |  end
          ^^^

test/testdata/parser/error_recovery/block_do_1.rb:24: Hint: this "do" token might not be properly closed https://srb.help/2003
    24 |    f() do # error: Hint: this "do" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/block_do_1.rb:26: Matching `end` found here but is not indented as far
    26 |  end
          ^^^
Errors: 5
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/block_do_1.rb:28: expected an `end` to close the `def` statement https://srb.help/2001
    28 |
        ^

test/testdata/parser/error_recovery/block_do_1.rb:28: expected an `end` to close the `def` statement https://srb.help/2001
    28 |
        ^

test/testdata/parser/error_recovery/block_do_1.rb:28: expected an `end` to close the `def` statement https://srb.help/2001
    28 |
        ^

test/testdata/parser/error_recovery/block_do_1.rb:28: expected an `end` to close the `class` statement https://srb.help/2001
    28 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the root cause entirely by not detecting the unclosed do blocks, instead only reporting generic missing end statements at EOF without the helpful indentation hints that pinpoint each problematic do token.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x().f() do ||
          <self>.puts("after")
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x().f() do ||
          <self>.puts("after")
        end
      end
    end

    def test3<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.f() do ||
          <self>.puts("after")
        end
      end
    end

    def test4<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.f() do ||
          <self>.puts("after")
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x().f() do ||
          <self>.puts("after")
        end
        def test2<<todo method>>(&<blk>)
          begin
            <self>.puts("before")
            <self>.x().f() do ||
              <self>.puts("after")
            end
            def test3<<todo method>>(&<blk>)
              begin
                <self>.puts("before")
                <self>.f() do ||
                  <self>.puts("after")
                end
                def test4<<todo method>>(&<blk>)
                  begin
                    <self>.puts("before")
                    <self>.f() do ||
                      <self>.puts("after")
                    end
                  end
                end
                <emptyTree>::<C <ErrorNode>>
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -6,34 +6,32 @@
         <self>.x().f() do ||
           <self>.puts("after")
         end
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <self>.x().f() do ||
-          <self>.puts("after")
+        def test2<<todo method>>(&<blk>)
+          begin
+            <self>.puts("before")
+            <self>.x().f() do ||
+              <self>.puts("after")
+            end
+            def test3<<todo method>>(&<blk>)
+              begin
+                <self>.puts("before")
+                <self>.f() do ||
+                  <self>.puts("after")
+                end
+                def test4<<todo method>>(&<blk>)
+                  begin
+                    <self>.puts("before")
+                    <self>.f() do ||
+                      <self>.puts("after")
+                    end
+                  end
+                end
+                <emptyTree>::<C <ErrorNode>>
+              end
+            end
+          end
         end
       end
     end
-
-    def test3<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <self>.f() do ||
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test4<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <self>.f() do ||
-          <self>.puts("after")
-        end
-      end
-    end
   end
 end
```

</details>

## case_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    # These errors are currently doubled because we actually report this error
    # on the first and second parse. (Most of the time, e.g. in files where
    # this is the only parse error, this syntax error alone won't force us into
    # indentation-aware recovery mode, so I've lazily decided this is good
    # enough. If you've made it better that's great!)
    puts 'before'
    case
  # ^^^^ error: "case" statement must at least have one "when" clause
  # ^^^^ error: Hint: this "case" token might not be properly closed
  # ^^^^ error: Hint: this "case" token might not be properly closed
  end

  def test2
    puts 'before'
    case x # error: unexpected token "case"
    puts 'after'
  end

  def test3
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    when
    puts 'after' # error: unexpected token tSTRING
  end

  def test4
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    when A
    puts 'after'
  end

  def test5
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    when A then
    puts 'after'
  end

  def test6
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    when then # error: unexpected token "then"
    puts 'after'
  end

  def test7
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    when A
    else
    puts 'after'
  end

  def test8
    puts 'before'
    case # error: Hint: this "case" token might not be properly closed
    when A
    else
    puts 'after'
  end

  def test9
    puts 'before'
    case x # error: Hint: this "case" token might not be properly closed
    in A
    else
    puts 'after'
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (14)</summary>

```
test/testdata/parser/error_recovery/case_1.rb:11: "case" statement must at least have one "when" clause https://srb.help/2001
    11 |    case
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:11: Hint: this "case" token might not be properly closed https://srb.help/2003
    11 |    case
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:15: Matching `end` found here but is not indented as far
    15 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:19: unexpected token "case" https://srb.help/2001
    19 |    case x # error: unexpected token "case"
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:27: unexpected token tSTRING https://srb.help/2001
    27 |    puts 'after' # error: unexpected token tSTRING
                 ^^^^^^^

test/testdata/parser/error_recovery/case_1.rb:47: unexpected token "then" https://srb.help/2001
    47 |    when then # error: unexpected token "then"
                 ^^^^

test/testdata/parser/error_recovery/case_1.rb:74: unexpected token "end of file" https://srb.help/2001
    74 |end # error: unexpected token "end of file"
    75 |

test/testdata/parser/error_recovery/case_1.rb:11: Hint: this "case" token might not be properly closed https://srb.help/2003
    11 |    case
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:15: Matching `end` found here but is not indented as far
    15 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:25: Hint: this "case" token might not be properly closed https://srb.help/2003
    25 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:28: Matching `end` found here but is not indented as far
    28 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:32: Hint: this "case" token might not be properly closed https://srb.help/2003
    32 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:35: Matching `end` found here but is not indented as far
    35 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:39: Hint: this "case" token might not be properly closed https://srb.help/2003
    39 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:42: Matching `end` found here but is not indented as far
    42 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:46: Hint: this "case" token might not be properly closed https://srb.help/2003
    46 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:49: Matching `end` found here but is not indented as far
    49 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:53: Hint: this "case" token might not be properly closed https://srb.help/2003
    53 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:57: Matching `end` found here but is not indented as far
    57 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:61: Hint: this "case" token might not be properly closed https://srb.help/2003
    61 |    case # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:65: Matching `end` found here but is not indented as far
    65 |  end
          ^^^

test/testdata/parser/error_recovery/case_1.rb:69: Hint: this "case" token might not be properly closed https://srb.help/2003
    69 |    case x # error: Hint: this "case" token might not be properly closed
            ^^^^
    test/testdata/parser/error_recovery/case_1.rb:73: Matching `end` found here but is not indented as far
    73 |  end
          ^^^
Errors: 14
```

</details>

<details>
<summary>Prism errors (14)</summary>

```
test/testdata/parser/error_recovery/case_1.rb:11: expected a `when` or `in` clause after `case` https://srb.help/2001
    11 |    case
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:19: expected a `when` or `in` clause after `case` https://srb.help/2001
    19 |    case x # error: unexpected token "case"
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:20: expected an `end` to close the `case` statement https://srb.help/2001
    20 |    puts 'after'
        ^

test/testdata/parser/error_recovery/case_1.rb:20: unexpected local variable or method, expecting end-of-input https://srb.help/2001
    20 |    puts 'after'
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:27: expected a delimiter after the predicates of a `when` clause https://srb.help/2001
    27 |    puts 'after' # error: unexpected token tSTRING
                ^

test/testdata/parser/error_recovery/case_1.rb:47: expected an expression after `when` https://srb.help/2001
    47 |    when then # error: unexpected token "then"
            ^^^^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `def` statement https://srb.help/2001
    75 |
        ^

test/testdata/parser/error_recovery/case_1.rb:75: expected an `end` to close the `class` statement https://srb.help/2001
    75 |
        ^
Errors: 14
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same number of errors with clearer messages for case statement issues, but points to EOF for multiple missing ends instead of the problematic case tokens, making it harder to locate the actual source of problems.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <emptyTree>::<C <ErrorNode>>
          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
            <emptyTree>
          else
            <emptyTree>
          end
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      <emptyTree>
    end

    def test3<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
            <emptyTree>
          else
            <emptyTree>
          end
        end
      end
    end

    def test4<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if <emptyTree>::<C A>.===(<assignTemp>$2)
            <self>.puts("after")
          else
            <emptyTree>
          end
        end
      end
    end

    def test5<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if <emptyTree>::<C A>.===(<assignTemp>$2)
            <self>.puts("after")
          else
            <emptyTree>
          end
        end
      end
    end

    def test6<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
            <self>.puts("after")
          else
            <emptyTree>
          end
        end
      end
    end

    def test7<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if <emptyTree>::<C A>.===(<assignTemp>$2)
            <emptyTree>
          else
            <self>.puts("after")
          end
        end
      end
    end

    def test8<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <emptyTree>::<C A>
          <emptyTree>
        else
          <self>.puts("after")
        end
      end
    end

    def test9<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        begin
          <assignTemp>$2 = <self>.x()
          if ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
            <emptyTree>
          else
            <self>.puts("after")
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <emptyTree>
        def test2<<todo method>>(&<blk>)
          begin
            <self>.puts("before")
            begin
              <assignTemp>$1 = <self>.x()
              <emptyTree>
            end
            <self>.puts("after")
          end
        end
        def test3<<todo method>>(&<blk>)
          begin
            <self>.puts("before")
            begin
              <assignTemp>$1 = <self>.x()
              if <self>.puts().===(<assignTemp>$1)
                "after"
              else
                <emptyTree>
              end
            end
            def test4<<todo method>>(&<blk>)
              begin
                <self>.puts("before")
                begin
                  <assignTemp>$1 = <self>.x()
                  if <emptyTree>::<C A>.===(<assignTemp>$1)
                    <self>.puts("after")
                  else
                    <emptyTree>
                  end
                end
                def test5<<todo method>>(&<blk>)
                  begin
                    <self>.puts("before")
                    begin
                      <assignTemp>$1 = <self>.x()
                      if <emptyTree>::<C A>.===(<assignTemp>$1)
                        <self>.puts("after")
                      else
                        <emptyTree>
                      end
                    end
                    def test6<<todo method>>(&<blk>)
                      begin
                        <self>.puts("before")
                        begin
                          <assignTemp>$2 = <self>.x()
                          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
                            <self>.puts("after")
                          else
                            <emptyTree>
                          end
                        end
                        def test7<<todo method>>(&<blk>)
                          begin
                            <self>.puts("before")
                            begin
                              <assignTemp>$1 = <self>.x()
                              if <emptyTree>::<C A>.===(<assignTemp>$1)
                                <emptyTree>
                              else
                                <self>.puts("after")
                              end
                            end
                            def test8<<todo method>>(&<blk>)
                              begin
                                <self>.puts("before")
                                if <emptyTree>::<C A>
                                  <emptyTree>
                                else
                                  <self>.puts("after")
                                end
                                def test9<<todo method>>(&<blk>)
                                  begin
                                    <self>.puts("before")
                                    begin
                                      <assignTemp>$1 = <self>.x()
                                      if ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
                                        <emptyTree>
                                      else
                                        <self>.puts("after")
                                      end
                                    end
                                  end
                                end
                                <emptyTree>::<C <ErrorNode>>
                              end
                            end
                          end
                        end
                      end
                    end
                  end
                end
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -3,114 +3,107 @@
     def test1<<todo method>>(&<blk>)
       begin
         <self>.puts("before")
-        begin
-          <assignTemp>$2 = <emptyTree>::<C <ErrorNode>>
-          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
-            <emptyTree>
-          else
-            <emptyTree>
-          end
-        end
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-
-    def test3<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
-            <emptyTree>
-          else
-            <emptyTree>
-          end
-        end
-      end
-    end
-
-    def test4<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if <emptyTree>::<C A>.===(<assignTemp>$2)
+        <emptyTree>
+        def test2<<todo method>>(&<blk>)
+          begin
+            <self>.puts("before")
+            begin
+              <assignTemp>$1 = <self>.x()
+              <emptyTree>
+            end
             <self>.puts("after")
-          else
-            <emptyTree>
           end
         end
-      end
-    end
-
-    def test5<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if <emptyTree>::<C A>.===(<assignTemp>$2)
-            <self>.puts("after")
-          else
-            <emptyTree>
+        def test3<<todo method>>(&<blk>)
+          begin
+            <self>.puts("before")
+            begin
+              <assignTemp>$1 = <self>.x()
+              if <self>.puts().===(<assignTemp>$1)
+                "after"
+              else
+                <emptyTree>
+              end
+            end
+            def test4<<todo method>>(&<blk>)
+              begin
+                <self>.puts("before")
+                begin
+                  <assignTemp>$1 = <self>.x()
+                  if <emptyTree>::<C A>.===(<assignTemp>$1)
+                    <self>.puts("after")
+                  else
+                    <emptyTree>
+                  end
+                end
+                def test5<<todo method>>(&<blk>)
+                  begin
+                    <self>.puts("before")
+                    begin
+                      <assignTemp>$1 = <self>.x()
+                      if <emptyTree>::<C A>.===(<assignTemp>$1)
+                        <self>.puts("after")
+                      else
+                        <emptyTree>
+                      end
+                    end
+                    def test6<<todo method>>(&<blk>)
+                      begin
+                        <self>.puts("before")
+                        begin
+                          <assignTemp>$2 = <self>.x()
+                          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
+                            <self>.puts("after")
+                          else
+                            <emptyTree>
+                          end
+                        end
+                        def test7<<todo method>>(&<blk>)
+                          begin
+                            <self>.puts("before")
+                            begin
+                              <assignTemp>$1 = <self>.x()
+                              if <emptyTree>::<C A>.===(<assignTemp>$1)
+                                <emptyTree>
+                              else
+                                <self>.puts("after")
+                              end
+                            end
+                            def test8<<todo method>>(&<blk>)
+                              begin
+                                <self>.puts("before")
+                                if <emptyTree>::<C A>
+                                  <emptyTree>
+                                else
+                                  <self>.puts("after")
+                                end
+                                def test9<<todo method>>(&<blk>)
+                                  begin
+                                    <self>.puts("before")
+                                    begin
+                                      <assignTemp>$1 = <self>.x()
+                                      if ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
+                                        <emptyTree>
+                                      else
+                                        <self>.puts("after")
+                                      end
+                                    end
+                                  end
+                                end
+                                <emptyTree>::<C <ErrorNode>>
+                              end
+                            end
+                          end
+                        end
+                      end
+                    end
+                  end
+                end
+              end
+            end
           end
         end
       end
     end
-
-    def test6<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
-            <self>.puts("after")
-          else
-            <emptyTree>
-          end
-        end
-      end
-    end
-
-    def test7<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if <emptyTree>::<C A>.===(<assignTemp>$2)
-            <emptyTree>
-          else
-            <self>.puts("after")
-          end
-        end
-      end
-    end
-
-    def test8<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        if <emptyTree>::<C A>
-          <emptyTree>
-        else
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test9<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        begin
-          <assignTemp>$2 = <self>.x()
-          if ::T.unsafe(::Kernel).raise("Sorbet rewriter pass partially unimplemented")
-            <emptyTree>
-          else
-            <self>.puts("after")
-          end
-        end
-      end
-    end
   end
 end
```

</details>

## case_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  puts 'before'

  def test1
    case # error: "case" statement must at least have one "when" clause
    end
  end

  def test2
    case x # error: "case" statement must at least have one "when" clause
    end
  end

  puts 'after'
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/case_2.rb:7: "case" statement must at least have one "when" clause https://srb.help/2001
     7 |    case # error: "case" statement must at least have one "when" clause
            ^^^^

test/testdata/parser/error_recovery/case_2.rb:12: "case" statement must at least have one "when" clause https://srb.help/2001
    12 |    case x # error: "case" statement must at least have one "when" clause
            ^^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/case_2.rb:7: expected a `when` or `in` clause after `case` https://srb.help/2001
     7 |    case # error: "case" statement must at least have one "when" clause
            ^^^^

test/testdata/parser/error_recovery/case_2.rb:12: expected a `when` or `in` clause after `case` https://srb.help/2001
    12 |    case x # error: "case" statement must at least have one "when" clause
            ^^^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism errors are excellent with accurate locations and clearer, more specific error messages that mention both when and in clauses.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.puts("before")

    def test1<<todo method>>(&<blk>)
      begin
        <assignTemp>$2 = <emptyTree>::<C <ErrorNode>>
        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <assignTemp>$2 = <self>.x()
        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    <self>.puts("after")
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.puts("before")

    def test1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def test2<<todo method>>(&<blk>)
      begin
        <assignTemp>$1 = <self>.x()
        <emptyTree>
      end
    end

    <self>.puts("after")
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -3,24 +3,13 @@
     <self>.puts("before")
 
     def test1<<todo method>>(&<blk>)
-      begin
-        <assignTemp>$2 = <emptyTree>::<C <ErrorNode>>
-        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
+      <emptyTree>
     end
 
     def test2<<todo method>>(&<blk>)
       begin
-        <assignTemp>$2 = <self>.x()
-        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$2)
-          <emptyTree>
-        else
-          <emptyTree>
-        end
+        <assignTemp>$1 = <self>.x()
+        <emptyTree>
       end
     end
 
```

</details>

## circular_argument_reference.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

[1,2,3].each do |x = x + 1|
              #      ^      error: circular argument reference x
              # ^           error: unmatched "|"
              #           ^ error: missing arg to "|" operator
              #        ^    error: Method `+` does not exist
end

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/circular_argument_reference.rb:3: circular argument reference x https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                             ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:3: unmatched "|" in block argument list https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                        ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:3: missing arg to "|" operator https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                                  ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:3: Method `+` does not exist on `NilClass` https://srb.help/7003
     3 |[1,2,3].each do |x = x + 1|
                               ^
  Got `NilClass` originating from:
    test/testdata/parser/error_recovery/circular_argument_reference.rb:3: Possibly uninitialized (`NilClass`) in:
     3 |[1,2,3].each do |x = x + 1|
     4 |              #      ^      error: circular argument reference x
     5 |              # ^           error: unmatched "|"
     6 |              #           ^ error: missing arg to "|" operator
     7 |              #        ^    error: Method `+` does not exist
     8 |end
Errors: 4
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/circular_argument_reference.rb:3: circular argument reference - x https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                         ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:3: expected the block parameters to end with `|` https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                              ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:3: unexpected '+', ignoring it https://srb.help/2001
     3 |[1,2,3].each do |x = x + 1|
                               ^

test/testdata/parser/error_recovery/circular_argument_reference.rb:8: unexpected 'end'; expected an expression after the operator https://srb.help/2001
     8 |end
        ^^^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical type error about Method + not existing on NilClass and reports a confusing error at line 8 end instead of properly recovering from the malformed block parameter syntax.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do ||
    x = x.+(1).|(<emptyTree>::<C <ErrorNode>>)
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do |x = x|
    begin
      <emptyTree>::<C <ErrorNode>>
      1.|(<emptyTree>::<C <ErrorNode>>)
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,8 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  [1, 2, 3].each() do ||
-    x = x.+(1).|(<emptyTree>::<C <ErrorNode>>)
+  [1, 2, 3].each() do |x = x|
+    begin
+      <emptyTree>::<C <ErrorNode>>
+      1.|(<emptyTree>::<C <ErrorNode>>)
+    end
   end
 end
```

</details>

## class_indent_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A
  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_1.rb:8: unexpected token "end of file" https://srb.help/2001
     8 |end # error: unexpected token "end of file"
     9 |

test/testdata/parser/error_recovery/class_indent_1.rb:3: Hint: this "class" token might not be properly closed https://srb.help/2003
     3 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_1.rb:8: Matching `end` found here but is not indented as far
     8 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_1.rb:9: expected an `end` to close the `class` statement https://srb.help/2001
     9 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but points to EOF instead of the problematic class token and provides less helpful context than Original's indentation hint.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_indent_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class B
  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed
    puts 'hello'
    puts 'hello'

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_2.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/class_indent_2.rb:3: Hint: this "class" token might not be properly closed https://srb.help/2003
     3 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_2.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_2.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but points to EOF instead of the problematic class token and lacks the helpful indentation hint that Original provides.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")

      <self>.puts("hello")
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")

      <self>.puts("hello")

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,10 +4,12 @@
       <self>.puts("hello")
 
       <self>.puts("hello")
-    end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_indent_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class C
  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_3.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/class_indent_3.rb:3: Hint: this "class" token might not be properly closed https://srb.help/2003
     3 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_3.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_3.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but points to EOF instead of the problematic class token, and provides less helpful context than Original's indentation hint.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.sig() do ||
        <self>.void()
      end

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,15 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
     class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
-      <emptyTree>
-    end
+      <self>.sig() do ||
+        <self>.void()
+      end
 
-    <self>.sig() do ||
-      <self>.void()
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_indent_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class D
  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed
    puts 'hello'

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_4.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/class_indent_4.rb:3: Hint: this "class" token might not be properly closed https://srb.help/2003
     3 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_4.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_4.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but provides less helpful context by pointing to EOF rather than the problematic class token and its mismatched end location like Original does.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")

      <self>.sig() do ||
        <self>.void()
      end

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -2,14 +2,16 @@
   class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
     class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
       <self>.puts("hello")
-    end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
+      <self>.sig() do ||
+        <self>.void()
+      end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_indent_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class E
  sig {void}
  def method1
  end

  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_5.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/class_indent_5.rb:7: Hint: this "class" token might not be properly closed https://srb.help/2003
     7 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_5.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_5.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but points to EOF instead of the problematic class token and lacks the helpful indentation hint that Original provides.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_indent_6.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class F
  sig {void}
  def method1
  end

  class Inner
# ^^^^^ error: Hint: this "class" token might not be properly closed
    puts 'hello'
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/class_indent_6.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/class_indent_6.rb:7: Hint: this "class" token might not be properly closed https://srb.help/2003
     7 |  class Inner
          ^^^^^
    test/testdata/parser/error_recovery/class_indent_6.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/class_indent_6.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but points to EOF instead of the problematic class token and provides less helpful context about indentation mismatch compared to Original's detailed hint system.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
       <self>.puts("hello")
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## class_weird_newline_indent.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  class Inner < Object
# ^^^^^ error: Hint: this "class" token might not be properly closed

  def method2
  end
end

class B
  class Inner <
# ^^^^^ error: Hint: this "class" token might not be properly closed
    Object

  def method2
  end
end

class C
  class Inner <
# ^^^^^ error: Hint: this "class" token might not be properly closed
    Object
    puts 'hello'

  def method2
  end
end

class D
  class
# ^^^^^ error: Hint: this "class" token might not be properly closed
    Inner

  def method2
  end
end

class E
  class
# ^^^^^ error: Hint: this "class" token might not be properly closed
    Inner
    puts 'hello'

  def method2
  end
end

class F
  class
# ^^^^^ error: Hint: this "class" token might not be properly closed
    Inner <
      Object
    puts 'hello'

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (7)</summary>

```
test/testdata/parser/error_recovery/class_weird_newline_indent.rb:58: unexpected token "end of file" https://srb.help/2001
    58 |end # error: unexpected token "end of file"
    59 |

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:4: Hint: this "class" token might not be properly closed https://srb.help/2003
     4 |  class Inner < Object
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:9: Matching `end` found here but is not indented as far
     9 |end
        ^^^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:12: Hint: this "class" token might not be properly closed https://srb.help/2003
    12 |  class Inner <
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:18: Matching `end` found here but is not indented as far
    18 |end
        ^^^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:21: Hint: this "class" token might not be properly closed https://srb.help/2003
    21 |  class Inner <
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:28: Matching `end` found here but is not indented as far
    28 |end
        ^^^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:31: Hint: this "class" token might not be properly closed https://srb.help/2003
    31 |  class
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:37: Matching `end` found here but is not indented as far
    37 |end
        ^^^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:40: Hint: this "class" token might not be properly closed https://srb.help/2003
    40 |  class
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:47: Matching `end` found here but is not indented as far
    47 |end
        ^^^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:50: Hint: this "class" token might not be properly closed https://srb.help/2003
    50 |  class
          ^^^^^
    test/testdata/parser/error_recovery/class_weird_newline_indent.rb:58: Matching `end` found here but is not indented as far
    58 |end # error: unexpected token "end of file"
        ^^^
Errors: 7
```

</details>

<details>
<summary>Prism errors (6)</summary>

```
test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^

test/testdata/parser/error_recovery/class_weird_newline_indent.rb:59: expected an `end` to close the `class` statement https://srb.help/2001
    59 |
        ^
Errors: 6
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism reports all 6 errors at EOF line 59 instead of pointing to the actual problematic class tokens, making it much harder to locate and fix the issues compared to Original's precise error locations.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
      <self>.puts("hello")
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
      <self>.puts("hello")
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
      <self>.puts("hello")
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
      class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
        def method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end

      class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
        class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
          <self>.puts("hello")

          def method2<<todo method>>(&<blk>)
            <emptyTree>
          end
        end

        class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
          class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
            def method2<<todo method>>(&<blk>)
              <emptyTree>
            end
          end

          class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
            class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
              <self>.puts("hello")

              def method2<<todo method>>(&<blk>)
                <emptyTree>
              end
            end

            class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
              class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
                <self>.puts("hello")

                def method2<<todo method>>(&<blk>)
                  <emptyTree>
                end
              end

              <emptyTree>::<C <ErrorNode>>
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,61 +1,57 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-  end
+    class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
+      class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
+        def method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
 
-  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
-    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
-      <emptyTree>
-    end
+      class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
+        class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
+          <self>.puts("hello")
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-  end
+          def method2<<todo method>>(&<blk>)
+            <emptyTree>
+          end
+        end
 
-  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
-    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
-      <self>.puts("hello")
-    end
+        class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
+          class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
+            def method2<<todo method>>(&<blk>)
+              <emptyTree>
+            end
+          end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-  end
+          class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
+            class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
+              <self>.puts("hello")
 
-  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
-    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
-      <emptyTree>
-    end
+              def method2<<todo method>>(&<blk>)
+                <emptyTree>
+              end
+            end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-  end
+            class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
+              class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
+                <self>.puts("hello")
 
-  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
-    class <emptyTree>::<C Inner><<C <todo sym>>> < (::<todo sym>)
-      <self>.puts("hello")
-    end
+                def method2<<todo method>>(&<blk>)
+                  <emptyTree>
+                end
+              end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
+              <emptyTree>::<C <ErrorNode>>
+            end
+          end
+        end
+      end
     end
   end
-
-  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
-    class <emptyTree>::<C Inner><<C <todo sym>>> < (<emptyTree>::<C Object>)
-      <self>.puts("hello")
-    end
-
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-  end
 end
```

</details>

## const_case.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

def foo(x)
  Integer::
 #       ^^ error: expected constant name following "::"
  case x
  when Integer
  end
end

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/const_case.rb:4: expected constant name following "::" https://srb.help/2001
     4 |  Integer::
                 ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/const_case.rb:7: unexpected 'when', ignoring it https://srb.help/2001
     7 |  when Integer
          ^^^^

test/testdata/parser/error_recovery/const_case.rb:9: unexpected 'end', ignoring it https://srb.help/2001
     9 |end
        ^^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual error (incomplete constant reference on line 4) and instead reports cascading errors on valid case/when/end syntax.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(x, &<blk>)
    begin
      <emptyTree>::<C Integer>::<C <ConstantNameMissing>>
      begin
        <assignTemp>$2 = x
        if <emptyTree>::<C Integer>.===(<assignTemp>$2)
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(x, &<blk>)
    begin
      <emptyTree>::<C Integer>.case(x)
      <emptyTree>::<C <ErrorNode>>
      <emptyTree>::<C Integer>
    end
  end

  <emptyTree>::<C <ErrorNode>>
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def foo<<todo method>>(x, &<blk>)
     begin
-      <emptyTree>::<C Integer>::<C <ConstantNameMissing>>
-      begin
-        <assignTemp>$2 = x
-        if <emptyTree>::<C Integer>.===(<assignTemp>$2)
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
+      <emptyTree>::<C Integer>.case(x)
+      <emptyTree>::<C <ErrorNode>>
+      <emptyTree>::<C Integer>
     end
   end
+
+  <emptyTree>::<C <ErrorNode>>
 end
```

</details>

## constant_only_scope.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A; end
def test_constant_only_scope
  A::
  #^^ error: expected constant name following "::"
end

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/constant_only_scope.rb:4: expected constant name following "::" https://srb.help/2001
     4 |  A::
           ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/constant_only_scope.rb:7: expected an `end` to close the `def` statement https://srb.help/2001
     7 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual error (incomplete constant scope resolution A::) and instead reports a misleading missing end error at EOF, while Original correctly identifies and locates the incomplete constant expression.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <emptyTree>
  end

  def test_constant_only_scope<<todo method>>(&<blk>)
    <emptyTree>::<C A>::<C <ConstantNameMissing>>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <emptyTree>
  end

  def test_constant_only_scope<<todo method>>(&<blk>)
    begin
      <emptyTree>::<C A>.end()
      <emptyTree>::<C <ErrorNode>>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,6 +4,9 @@
   end
 
   def test_constant_only_scope<<todo method>>(&<blk>)
-    <emptyTree>::<C A>::<C <ConstantNameMissing>>
+    begin
+      <emptyTree>::<C A>.end()
+      <emptyTree>::<C <ErrorNode>>
+    end
   end
 end
```

</details>

## csend_masgn.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

[1,2,3].each do |x|
  x&.y, = 10
 # ^^  error: &. inside multiple assignment
 # ^^  error: Used `&.` operator on `Integer`
 #   ^ error: Setter method `y=` does not exist
end

```

</details>

<details>
<summary>Original errors (3) | Autocorrects: 1</summary>

```
test/testdata/parser/error_recovery/csend_masgn.rb:4: &. inside multiple assignment destination https://srb.help/2001
     4 |  x&.y, = 10
           ^^

test/testdata/parser/error_recovery/csend_masgn.rb:4: Used `&.` operator on `Integer`, which can never be nil https://srb.help/7034
     4 |  x&.y, = 10
           ^^
  Got `Integer` originating from:
    test/testdata/parser/error_recovery/csend_masgn.rb:3:
     3 |[1,2,3].each do |x|
                         ^
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/csend_masgn.rb:4: Replace with `.`
     4 |  x&.y, = 10
           ^^

test/testdata/parser/error_recovery/csend_masgn.rb:4: Setter method `y=` does not exist on `Integer` https://srb.help/7003
     4 |  x&.y, = 10
             ^
  Got `Integer` originating from:
    test/testdata/parser/error_recovery/csend_masgn.rb:3:
     3 |[1,2,3].each do |x|
                         ^
    test/testdata/parser/error_recovery/csend_masgn.rb:4:
     4 |  x&.y, = 10
          ^
  Did you mean `<=`? Use `-a` to autocorrect
    test/testdata/parser/error_recovery/csend_masgn.rb:4: Replace with `<=`
     4 |  x&.y, = 10
             ^
    https://github.com/sorbet/sorbet/tree/master/rbi/core/integer.rbi#L334: Defined here
     334 |  def <=(arg0); end
            ^^^^^^^^^^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (3) | Autocorrects: 1</summary>

```
test/testdata/parser/error_recovery/csend_masgn.rb:4: &. inside multiple assignment destination https://srb.help/2001
     4 |  x&.y, = 10
          ^^^^

test/testdata/parser/error_recovery/csend_masgn.rb:4: Used `&.` operator on `Integer`, which can never be nil https://srb.help/7034
     4 |  x&.y, = 10
           ^^
  Got `Integer` originating from:
    test/testdata/parser/error_recovery/csend_masgn.rb:3:
     3 |[1,2,3].each do |x|
                         ^
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/csend_masgn.rb:4: Replace with `.`
     4 |  x&.y, = 10
           ^^

test/testdata/parser/error_recovery/csend_masgn.rb:4: Setter method `y=` does not exist on `Integer` https://srb.help/7003
     4 |  x&.y, = 10
             ^
  Got `Integer` originating from:
    test/testdata/parser/error_recovery/csend_masgn.rb:3:
     3 |[1,2,3].each do |x|
                         ^
    test/testdata/parser/error_recovery/csend_masgn.rb:4:
     4 |  x&.y, = 10
          ^
  Did you mean `<=`? Use `-a` to autocorrect
    test/testdata/parser/error_recovery/csend_masgn.rb:4: Replace with `<=`
     4 |  x&.y, = 10
             ^
    https://github.com/sorbet/sorbet/tree/master/rbi/core/integer.rbi#L334: Defined here
     334 |  def <=(arg0); end
            ^^^^^^^^^^^^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all the same errors but the first error underlines the entire expression x&.y instead of just the &. operator, making it slightly less precise than Original.

</details>

<details>
<summary>Prism autocorrect diff</summary>

```diff
--- Original
+++ Autocorrected
@@ -1,7 +1,7 @@
 # typed: true
 
 [1,2,3].each do |x|
-  x&.y, = 10
+  x.<=, = 10
  # ^^  error: &. inside multiple assignment
  # ^^  error: Used `&.` operator on `Integer`
  #   ^ error: Setter method `y=` does not exist
```

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do |x|
    begin
      <assignTemp>$2 = 10
      <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 1, 0)
      begin
        <assignTemp>$4 = x
        if ::NilClass.===(<assignTemp>$4)
          ::<Magic>.<nil-for-safe-navigation>(<assignTemp>$4)
        else
          <assignTemp>$4.y=(<assignTemp>$3.[](0))
        end
      end
      <assignTemp>$2
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].each() do |x|
    begin
      <assignTemp>$2 = 10
      <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 1, 0)
      begin
        <assignTemp>$4 = x
        if ::NilClass.===(<assignTemp>$4)
          ::<Magic>.<nil-for-safe-navigation>(<assignTemp>$4)
        else
          <assignTemp>$4.y=(<assignTemp>$3.[](0))
        end
      end
      <assignTemp>$2
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## def_missing_end_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
# ^^^ error: Hint: this "def" token might not be properly closed
    if x.f
    end

  def test2
    if x.f
    end
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/def_missing_end_1.rb:13: unexpected token "end of file" https://srb.help/2001
    13 |end # error: unexpected token "end of file"
    14 |

test/testdata/parser/error_recovery/def_missing_end_1.rb:4: Hint: this "def" token might not be properly closed https://srb.help/2003
     4 |  def test1
          ^^^
    test/testdata/parser/error_recovery/def_missing_end_1.rb:13: Matching `end` found here but is not indented as far
    13 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/def_missing_end_1.rb:14: expected an `end` to close the `class` statement https://srb.help/2001
    14 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (missing end for test1 method) and only reports the class-level issue, providing less helpful diagnostics than Original which correctly identifies the unclosed def token.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      if <self>.x().f()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test2<<todo method>>(&<blk>)
      if <self>.x().f()
        <emptyTree>
      else
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        if <self>.x().f()
          <emptyTree>
        else
          <emptyTree>
        end
        def test2<<todo method>>(&<blk>)
          if <self>.x().f()
            <emptyTree>
          else
            <emptyTree>
          end
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,19 +1,22 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def test1<<todo method>>(&<blk>)
-      if <self>.x().f()
-        <emptyTree>
-      else
-        <emptyTree>
+      begin
+        if <self>.x().f()
+          <emptyTree>
+        else
+          <emptyTree>
+        end
+        def test2<<todo method>>(&<blk>)
+          if <self>.x().f()
+            <emptyTree>
+          else
+            <emptyTree>
+          end
+        end
       end
     end
 
-    def test2<<todo method>>(&<blk>)
-      if <self>.x().f()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## def_missing_end_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    if x.f
    end
  end

  def test2
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'before'
    if x
  # ^^ error: Hint: this "if" token might not be properly closed
    puts 'after'
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/def_missing_end_2.rb:15: unexpected token "end of file" https://srb.help/2001
    15 |end # error: unexpected token "end of file"
    16 |

test/testdata/parser/error_recovery/def_missing_end_2.rb:12: Hint: this "if" token might not be properly closed https://srb.help/2003
    12 |    if x
            ^^
    test/testdata/parser/error_recovery/def_missing_end_2.rb:15: Matching `end` found here but is not indented as far
    15 |end # error: unexpected token "end of file"
        ^^^

test/testdata/parser/error_recovery/def_missing_end_2.rb:9: Hint: this "def" token might not be properly closed https://srb.help/2003
     9 |  def test2
          ^^^
    test/testdata/parser/error_recovery/def_missing_end_2.rb:15: Matching `end` found here but is not indented as far
    15 |end # error: unexpected token "end of file"
        ^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/def_missing_end_2.rb:16: expected an `end` to close the `def` statement https://srb.help/2001
    16 |
        ^

test/testdata/parser/error_recovery/def_missing_end_2.rb:16: expected an `end` to close the `class` statement https://srb.help/2001
    16 |
        ^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical unclosed if statement error and provides less helpful diagnostics by only pointing to EOF instead of identifying the specific problematic tokens with indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      if <self>.x().f()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <self>.x()
          <self>.puts("after")
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      if <self>.x().f()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <self>.x()
          <self>.puts("after")
        else
          <emptyTree>
        end
        <emptyTree>::<C <ErrorNode>>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -16,6 +16,7 @@
         else
           <emptyTree>
         end
+        <emptyTree>::<C <ErrorNode>>
       end
     end
   end
```

</details>

## defn_indent_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A
  def method1
# ^^^ error: Hint: this "def" token might not be properly closed

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_1.rb:8: unexpected token "end of file" https://srb.help/2001
     8 |end # error: unexpected token "end of file"
     9 |

test/testdata/parser/error_recovery/defn_indent_1.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def method1
          ^^^
    test/testdata/parser/error_recovery/defn_indent_1.rb:8: Matching `end` found here but is not indented as far
     8 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_1.rb:9: expected an `end` to close the `class` statement https://srb.help/2001
     9 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (unclosed method1 definition) and only reports the missing class end, providing less helpful diagnostics than Original which correctly identifies the improperly closed def token with indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(&<blk>)
      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def method1<<todo method>>(&<blk>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defn_indent_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class B
  def method1(abc, xyz)
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'
    puts 'hello'

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_2.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defn_indent_2.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def method1(abc, xyz)
          ^^^
    test/testdata/parser/error_recovery/defn_indent_2.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_2.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation hint about the unclosed def token and only reports a generic missing end error, providing less actionable debugging information than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
      end
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
        def method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,11 +4,12 @@
       begin
         <self>.puts("hello")
         <self>.puts("hello")
+        def method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
       end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defn_indent_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class C
  def method1
# ^^^ error: Hint: this "def" token might not be properly closed

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_3.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/defn_indent_3.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def method1
          ^^^
    test/testdata/parser/error_recovery/defn_indent_3.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_3.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the primary issue (unclosed method1) and only reports the secondary consequence (unclosed class), providing less actionable debugging information than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
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

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,16 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
     def method1<<todo method>>(&<blk>)
-      <emptyTree>
+      begin
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defn_indent_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class D
  def method1
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_4.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defn_indent_4.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def method1
          ^^^
    test/testdata/parser/error_recovery/defn_indent_4.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_4.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation mismatch error for method1's unclosed def and only reports a generic missing end for the class, providing less actionable debugging information than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    def method1<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
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

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,17 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
     def method1<<todo method>>(&<blk>)
-      <self>.puts("hello")
+      begin
+        <self>.puts("hello")
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defn_indent_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class E
  sig {void}
  def method1
  end

  def method2
# ^^^ error: Hint: this "def" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_5.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/defn_indent_5.rb:7: Hint: this "def" token might not be properly closed https://srb.help/2003
     7 |  def method2
          ^^^
    test/testdata/parser/error_recovery/defn_indent_5.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_5.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (unclosed method2 definition) and only reports the missing class end, while Original correctly identifies the improperly closed def token with helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     def method2<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defn_indent_6.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class F
  sig {void}
  def method1
  end

  def method2
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defn_indent_6.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defn_indent_6.rb:7: Hint: this "def" token might not be properly closed https://srb.help/2003
     7 |  def method2
          ^^^
    test/testdata/parser/error_recovery/defn_indent_6.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defn_indent_6.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (method2 not properly closed) and only reports the class-level issue, while Original correctly identifies the unclosed def and provides helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     def method2<<todo method>>(&<blk>)
       <self>.puts("hello")
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A
  def self.method1
# ^^^ error: Hint: this "def" token might not be properly closed

  def self.method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_1.rb:8: unexpected token "end of file" https://srb.help/2001
     8 |end # error: unexpected token "end of file"
     9 |

test/testdata/parser/error_recovery/defs_indent_1.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def self.method1
          ^^^
    test/testdata/parser/error_recovery/defs_indent_1.rb:8: Matching `end` found here but is not indented as far
     8 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_1.rb:9: expected an `end` to close the `class` statement https://srb.help/2001
     9 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the key indentation hint about the unclosed def on line 3 and only reports the missing class end, providing less helpful diagnostic information than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      def self.method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def self.method1<<todo method>>(&<blk>)
-      <emptyTree>
+      def self.method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def self.method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class B
  def self.method1(abc, xyz)
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'
    puts 'hello'

  def self.method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_2.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defs_indent_2.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def self.method1(abc, xyz)
          ^^^
    test/testdata/parser/error_recovery/defs_indent_2.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_2.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation mismatch error that identifies the actual problem (method1 missing its end), only reporting a generic class end missing error.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
      end
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(abc, xyz, &<blk>)
      begin
        <self>.puts("hello")
        <self>.puts("hello")
        def self.method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,11 +4,12 @@
       begin
         <self>.puts("hello")
         <self>.puts("hello")
+        def self.method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
       end
     end
 
-    def self.method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class C
  def self.method1
# ^^^ error: Hint: this "def" token might not be properly closed

  sig {void}
  def self.method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_3.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/defs_indent_3.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def self.method1
          ^^^
    test/testdata/parser/error_recovery/defs_indent_3.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_3.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation hint about the unclosed def on line 3 and only reports the missing class end, providing less actionable debugging information than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      begin
        <self>.sig() do ||
          <self>.void()
        end
        def self.method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,16 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
     def self.method1<<todo method>>(&<blk>)
-      <emptyTree>
+      begin
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def self.method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def self.method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class D
  def self.method1
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'

  sig {void}
  def self.method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_4.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defs_indent_4.rb:3: Hint: this "def" token might not be properly closed https://srb.help/2003
     3 |  def self.method1
          ^^^
    test/testdata/parser/error_recovery/defs_indent_4.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_4.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation mismatch error that identifies the actual problem with method1's unclosed def statement, only reporting a generic missing end error.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      <self>.puts("hello")
    end

    <self>.sig() do ||
      <self>.void()
    end

    def self.method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    def self.method1<<todo method>>(&<blk>)
      begin
        <self>.puts("hello")
        <self>.sig() do ||
          <self>.void()
        end
        def self.method2<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,17 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
     def self.method1<<todo method>>(&<blk>)
-      <self>.puts("hello")
+      begin
+        <self>.puts("hello")
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def self.method2<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def self.method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class E
  sig {void}
  def self.method1
  end

  def self.method2
# ^^^ error: Hint: this "def" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_5.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/defs_indent_5.rb:7: Hint: this "def" token might not be properly closed https://srb.help/2003
     7 |  def self.method2
          ^^^
    test/testdata/parser/error_recovery/defs_indent_5.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_5.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (unclosed method2 definition) and only reports the missing class end, while Original correctly identifies the improperly closed def token with helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

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

</details>

<details>
<summary>Prism desugar tree</summary>

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

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     def self.method2<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## defs_indent_6.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class F
  sig {void}
  def self.method1
  end

  def self.method2
# ^^^ error: Hint: this "def" token might not be properly closed
    puts 'hello'
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/defs_indent_6.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/defs_indent_6.rb:7: Hint: this "def" token might not be properly closed https://srb.help/2003
     7 |  def self.method2
          ^^^
    test/testdata/parser/error_recovery/defs_indent_6.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/defs_indent_6.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical indentation mismatch error that Original correctly identifies, only reporting the missing class end without diagnosing the actual def/end pairing problem.

</details>

<details>
<summary>Original desugar tree</summary>

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

</details>

<details>
<summary>Prism desugar tree</summary>

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

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     def self.method2<<todo method>>(&<blk>)
       <self>.puts("hello")
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## eof_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

class A # error: Hint: this "class" token is not closed before the end of the file
  extend T::Sig

  sig {params(x: Integer).void}
  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
    puts(x)

  # We get a double-reported error here because of the naive/greedy
  # backtracking that we do to recover. The `def bar` is parsed, then we
  # attempt to parse the `def foo`, end up backtracking to just after the
  # puts(x), and then we parse the `def bar` a second time.
  #
  # Reporting an error twice seems less bad than failing to parse entirely.
  sig {void}
  def bar
# ^^^ error: Hint: this "def" token is not closed before the end of the file
# ^^^ error: Hint: this "def" token is not closed before the end of the file

  puts 'after' # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/eof_1.rb:21: unexpected token "end of file" https://srb.help/2001
    21 |  puts 'after' # error: unexpected token "end of file"
    22 |

test/testdata/parser/error_recovery/eof_1.rb:17: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
    17 |  def bar
          ^^^

test/testdata/parser/error_recovery/eof_1.rb:7: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     7 |  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
          ^^^

test/testdata/parser/error_recovery/eof_1.rb:17: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
    17 |  def bar
          ^^^

test/testdata/parser/error_recovery/eof_1.rb:3: Hint: this "class" token is not closed before the end of the file https://srb.help/2003
     3 |class A # error: Hint: this "class" token is not closed before the end of the file
        ^^^^^
Errors: 5
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/eof_1.rb:22: expected an `end` to close the `def` statement https://srb.help/2001
    22 |
        ^

test/testdata/parser/error_recovery/eof_1.rb:22: expected an `end` to close the `def` statement https://srb.help/2001
    22 |
        ^

test/testdata/parser/error_recovery/eof_1.rb:22: expected an `end` to close the `class` statement https://srb.help/2001
    22 |
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all unclosed tokens but points to EOF instead of the actual opening tokens, making errors less actionable than Original's hints that identify the specific problematic lines.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def foo<<todo method>>(x, &<blk>)
      <self>.puts(x)
    end

    <self>.sig() do ||
      <self>.void()
    end

    def bar<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.puts("after")
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def foo<<todo method>>(x, &<blk>)
      begin
        <self>.puts(x)
        <self>.sig() do ||
          <self>.void()
        end
        def bar<<todo method>>(&<blk>)
          begin
            <self>.puts("after")
            <emptyTree>::<C <ErrorNode>>
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -7,17 +7,18 @@
     end
 
     def foo<<todo method>>(x, &<blk>)
-      <self>.puts(x)
+      begin
+        <self>.puts(x)
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def bar<<todo method>>(&<blk>)
+          begin
+            <self>.puts("after")
+            <emptyTree>::<C <ErrorNode>>
+          end
+        end
+      end
     end
-
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def bar<<todo method>>(&<blk>)
-      <emptyTree>
-    end
-
-    <self>.puts("after")
   end
 end
```

</details>

## eof_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

def foo # error: Hint: this "def" token is not closed before the end of the file
  def bar # error: Hint: this "def" token is not closed before the end
    def qux # error: Hint: this "def" token is not closed before the end
      puts('inside qux')

    puts('inside bar')

  puts('inside foo') # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_2.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |  puts('inside foo') # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/eof_2.rb:5: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     5 |    def qux # error: Hint: this "def" token is not closed before the end
            ^^^

test/testdata/parser/error_recovery/eof_2.rb:4: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     4 |  def bar # error: Hint: this "def" token is not closed before the end
          ^^^

test/testdata/parser/error_recovery/eof_2.rb:3: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     3 |def foo # error: Hint: this "def" token is not closed before the end of the file
        ^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/eof_2.rb:11: expected an `end` to close the `def` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_2.rb:11: expected an `end` to close the `def` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_2.rb:11: expected an `end` to close the `def` statement https://srb.help/2001
    11 |
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all three missing end tokens but points to EOF for all errors instead of the specific def tokens, and the identical messages make it harder to distinguish which def is unclosed.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    begin
      def bar<<todo method>>(&<blk>)
        begin
          def qux<<todo method>>(&<blk>)
            <self>.puts("inside qux")
          end
          <self>.puts("inside bar")
        end
      end
      <self>.puts("inside foo")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    def bar<<todo method>>(&<blk>)
      def qux<<todo method>>(&<blk>)
        begin
          <self>.puts("inside qux")
          <self>.puts("inside bar")
          <self>.puts("inside foo")
          <emptyTree>::<C <ErrorNode>>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,14 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def foo<<todo method>>(&<blk>)
-    begin
-      def bar<<todo method>>(&<blk>)
+    def bar<<todo method>>(&<blk>)
+      def qux<<todo method>>(&<blk>)
         begin
-          def qux<<todo method>>(&<blk>)
-            <self>.puts("inside qux")
-          end
+          <self>.puts("inside qux")
           <self>.puts("inside bar")
+          <self>.puts("inside foo")
+          <emptyTree>::<C <ErrorNode>>
         end
       end
-      <self>.puts("inside foo")
     end
   end
 end
```

</details>

## eof_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

module A # error: Hint: this "module" token is not closed before the end of the file
  module B # error: Hint: this "module" token is not closed before the end of the file
    module # error: Hint: this "module" token is not closed before the end of the file
      C # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_3.rb:6: unexpected token "end of file" https://srb.help/2001
     6 |      C # error: unexpected token "end of file"
     7 |

test/testdata/parser/error_recovery/eof_3.rb:5: Hint: this "module" token is not closed before the end of the file https://srb.help/2003
     5 |    module # error: Hint: this "module" token is not closed before the end of the file
            ^^^^^^

test/testdata/parser/error_recovery/eof_3.rb:4: Hint: this "module" token is not closed before the end of the file https://srb.help/2003
     4 |  module B # error: Hint: this "module" token is not closed before the end of the file
          ^^^^^^

test/testdata/parser/error_recovery/eof_3.rb:3: Hint: this "module" token is not closed before the end of the file https://srb.help/2003
     3 |module A # error: Hint: this "module" token is not closed before the end of the file
        ^^^^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/eof_3.rb:7: expected an `end` to close the `module` statement https://srb.help/2001
     7 |
        ^

test/testdata/parser/error_recovery/eof_3.rb:7: expected an `end` to close the `module` statement https://srb.help/2001
     7 |
        ^

test/testdata/parser/error_recovery/eof_3.rb:7: expected an `end` to close the `module` statement https://srb.help/2001
     7 |
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all three unclosed modules but points to EOF for all errors instead of the specific module tokens, and lacks the helpful context of which line C appears on that Original provides.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A><<C <todo sym>>> < ()
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      module <emptyTree>::<C C><<C <todo sym>>> < ()
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A><<C <todo sym>>> < ()
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      module <emptyTree>::<C C><<C <todo sym>>> < ()
        <emptyTree>::<C <ErrorNode>>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -2,7 +2,7 @@
   module <emptyTree>::<C A><<C <todo sym>>> < ()
     module <emptyTree>::<C B><<C <todo sym>>> < ()
       module <emptyTree>::<C C><<C <todo sym>>> < ()
-        <emptyTree>
+        <emptyTree>::<C <ErrorNode>>
       end
     end
   end
```

</details>

## eof_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

module A # error: Hint: this "module" token is not closed before the end of the file
  module B
    module C
    end
  end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/eof_4.rb:7: unexpected token "end of file" https://srb.help/2001
     7 |  end # error: unexpected token "end of file"
     8 |

test/testdata/parser/error_recovery/eof_4.rb:3: Hint: this "module" token is not closed before the end of the file https://srb.help/2003
     3 |module A # error: Hint: this "module" token is not closed before the end of the file
        ^^^^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/eof_4.rb:8: expected an `end` to close the `module` statement https://srb.help/2001
     8 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but only reports one error at EOF instead of two errors with a helpful hint pointing to the unclosed module A token.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A><<C <todo sym>>> < ()
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      module <emptyTree>::<C C><<C <todo sym>>> < ()
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A><<C <todo sym>>> < ()
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      module <emptyTree>::<C C><<C <todo sym>>> < ()
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -5,5 +5,7 @@
         <emptyTree>
       end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## eof_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

class A # error: Hint: this "class" token is not closed before the end of the file
  module B # error: Hint: this "module" token is not closed before the end of the file
    def self.foo # error: Hint: this "def" token is not closed before the end of the file
      def bar # error: Hint: this "def" token is not closed before the end of the file

      puts('inside foo')
  # This is currently nested in the wrong scope.
  puts('inside A') # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/eof_5.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |  puts('inside A') # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/eof_5.rb:6: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     6 |      def bar # error: Hint: this "def" token is not closed before the end of the file
              ^^^

test/testdata/parser/error_recovery/eof_5.rb:5: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     5 |    def self.foo # error: Hint: this "def" token is not closed before the end of the file
            ^^^

test/testdata/parser/error_recovery/eof_5.rb:4: Hint: this "module" token is not closed before the end of the file https://srb.help/2003
     4 |  module B # error: Hint: this "module" token is not closed before the end of the file
          ^^^^^^

test/testdata/parser/error_recovery/eof_5.rb:3: Hint: this "class" token is not closed before the end of the file https://srb.help/2003
     3 |class A # error: Hint: this "class" token is not closed before the end of the file
        ^^^^^
Errors: 5
```

</details>

<details>
<summary>Prism errors (6) | Autocorrects: 2</summary>

```
test/testdata/parser/error_recovery/eof_5.rb:11: expected an `end` to close the `def` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_5.rb:11: expected an `end` to close the `def` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_5.rb:11: expected an `end` to close the `module` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_5.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^

test/testdata/parser/error_recovery/eof_5.rb:8: Method `puts` does not exist on `A::B` https://srb.help/7003
     8 |      puts('inside foo')
              ^^^^
  Got `A::B` originating from:
    test/testdata/parser/error_recovery/eof_5.rb:6:
     6 |      def bar # error: Hint: this "def" token is not closed before the end of the file
              ^
  Note:
    `puts` is actually defined as a method on `Kernel`. To call it,
    `include Kernel` in this module to ensure the method is always there, or
    call the method using `Kernel.puts` instead.
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/eof_5.rb:8: Insert `Kernel.`
     8 |      puts('inside foo')
              ^

test/testdata/parser/error_recovery/eof_5.rb:10: Method `puts` does not exist on `A::B` https://srb.help/7003
    10 |  puts('inside A') # error: unexpected token "end of file"
          ^^^^
  Got `A::B` originating from:
    test/testdata/parser/error_recovery/eof_5.rb:6:
     6 |      def bar # error: Hint: this "def" token is not closed before the end of the file
              ^
  Note:
    `puts` is actually defined as a method on `Kernel`. To call it,
    `include Kernel` in this module to ensure the method is always there, or
    call the method using `Kernel.puts` instead.
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/eof_5.rb:10: Insert `Kernel.`
    10 |  puts('inside A') # error: unexpected token "end of file"
          ^
Errors: 6
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism incorrectly nests both puts calls inside def bar instead of their intended scopes, causing spurious type errors about puts not existing on A::B.

</details>

<details>
<summary>Prism autocorrect diff</summary>

```diff
--- Original
+++ Autocorrected
@@ -5,6 +5,6 @@
     def self.foo # error: Hint: this "def" token is not closed before the end of the file
       def bar # error: Hint: this "def" token is not closed before the end of the file
 
-      puts('inside foo')
+      Kernel.puts('inside foo')
   # This is currently nested in the wrong scope.
-  puts('inside A') # error: unexpected token "end of file"
+  Kernel.puts('inside A') # error: unexpected token "end of file"
```

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      def self.foo<<todo method>>(&<blk>)
        begin
          def bar<<todo method>>(&<blk>)
            <emptyTree>
          end
          <self>.puts("inside foo")
        end
      end

      <self>.puts("inside A")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C B><<C <todo sym>>> < ()
      def self.foo<<todo method>>(&<blk>)
        def bar<<todo method>>(&<blk>)
          begin
            <self>.puts("inside foo")
            <self>.puts("inside A")
            <emptyTree>::<C <ErrorNode>>
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -2,15 +2,14 @@
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     module <emptyTree>::<C B><<C <todo sym>>> < ()
       def self.foo<<todo method>>(&<blk>)
-        begin
-          def bar<<todo method>>(&<blk>)
-            <emptyTree>
+        def bar<<todo method>>(&<blk>)
+          begin
+            <self>.puts("inside foo")
+            <self>.puts("inside A")
+            <emptyTree>::<C <ErrorNode>>
           end
-          <self>.puts("inside foo")
         end
       end
-
-      <self>.puts("inside A")
     end
   end
 end
```

</details>

## eof_6.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

class A # error: Hint: this "class" token is not closed before the end of the file
  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
    if x # error: Hint: this "if" token is not closed before the end of the file
      puts(x) # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_6.rb:6: unexpected token "end of file" https://srb.help/2001
     6 |      puts(x) # error: unexpected token "end of file"
     7 |

test/testdata/parser/error_recovery/eof_6.rb:5: Hint: this "if" token is not closed before the end of the file https://srb.help/2003
     5 |    if x # error: Hint: this "if" token is not closed before the end of the file
            ^^

test/testdata/parser/error_recovery/eof_6.rb:4: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     4 |  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
          ^^^

test/testdata/parser/error_recovery/eof_6.rb:3: Hint: this "class" token is not closed before the end of the file https://srb.help/2003
     3 |class A # error: Hint: this "class" token is not closed before the end of the file
        ^^^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/eof_6.rb:7: expected an `end` to close the conditional clause https://srb.help/2001
     7 |
        ^

test/testdata/parser/error_recovery/eof_6.rb:7: expected an `end` to close the `def` statement https://srb.help/2001
     7 |
        ^

test/testdata/parser/error_recovery/eof_6.rb:7: expected an `end` to close the `class` statement https://srb.help/2001
     7 |
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all three unclosed constructs but points to EOF line 7 instead of the actual problematic tokens, and misses the specific error on line 6 where the issue manifests.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, &<blk>)
      if x
        <self>.puts(x)
      else
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, &<blk>)
      if x
        begin
          <self>.puts(x)
          <emptyTree>::<C <ErrorNode>>
        end
      else
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -2,7 +2,10 @@
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def foo<<todo method>>(x, &<blk>)
       if x
-        <self>.puts(x)
+        begin
+          <self>.puts(x)
+          <emptyTree>::<C <ErrorNode>>
+        end
       else
         <emptyTree>
       end
```

</details>

## eof_7.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

# TODO(jez) We don't do the same rewinding for begin/end blocks that we do for
# class/method blocks, so the indentation levels in the body of the begin are
# not used for the sake of backtracking at the moment.

class A # error: Hint: this "class" token is not closed before the end of the file
  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
    puts('outside begin')
    begin # error: Hint: this "begin" token is not closed before the end of the file
      puts('inside begin')

    puts('between')

    begin # error: Hint: this "begin" token is not closed before the end of the file
      puts('second begin')
    rescue
      puts('inside rescue') # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/eof_7.rb:18: unexpected token "end of file" https://srb.help/2001
    18 |      puts('inside rescue') # error: unexpected token "end of file"
    19 |

test/testdata/parser/error_recovery/eof_7.rb:15: Hint: this "begin" token is not closed before the end of the file https://srb.help/2003
    15 |    begin # error: Hint: this "begin" token is not closed before the end of the file
            ^^^^^

test/testdata/parser/error_recovery/eof_7.rb:10: Hint: this "begin" token is not closed before the end of the file https://srb.help/2003
    10 |    begin # error: Hint: this "begin" token is not closed before the end of the file
            ^^^^^

test/testdata/parser/error_recovery/eof_7.rb:8: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     8 |  def foo(x) # error: Hint: this "def" token is not closed before the end of the file
          ^^^

test/testdata/parser/error_recovery/eof_7.rb:7: Hint: this "class" token is not closed before the end of the file https://srb.help/2003
     7 |class A # error: Hint: this "class" token is not closed before the end of the file
        ^^^^^
Errors: 5
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_7.rb:19: expected an `end` to close the `begin` statement https://srb.help/2001
    19 |
        ^

test/testdata/parser/error_recovery/eof_7.rb:19: expected an `end` to close the `begin` statement https://srb.help/2001
    19 |
        ^

test/testdata/parser/error_recovery/eof_7.rb:19: expected an `end` to close the `def` statement https://srb.help/2001
    19 |
        ^

test/testdata/parser/error_recovery/eof_7.rb:19: expected an `end` to close the `class` statement https://srb.help/2001
    19 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all unclosed constructs but points all errors to EOF line 19 instead of the actual problematic token locations, and misses the primary error about the rescue block issue.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, &<blk>)
      begin
        <self>.puts("outside begin")
        begin
          <self>.puts("inside begin")
          <self>.puts("between")
          <self>.puts("second begin")
        rescue => <rescueTemp>$2
          <self>.puts("inside rescue")
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, &<blk>)
      begin
        <self>.puts("outside begin")
        begin
          <self>.puts("inside begin")
          <self>.puts("between")
          <self>.puts("second begin")
        rescue => <rescueTemp>$2
          begin
            <self>.puts("inside rescue")
            <emptyTree>::<C <ErrorNode>>
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -8,7 +8,10 @@
           <self>.puts("between")
           <self>.puts("second begin")
         rescue => <rescueTemp>$2
-          <self>.puts("inside rescue")
+          begin
+            <self>.puts("inside rescue")
+            <emptyTree>::<C <ErrorNode>>
+          end
         end
       end
     end
```

</details>

## eof_8.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
#        ^ to silence the dead code errors

# TODO(jez) We don't do the same rewinding for these constructs that we do for
# class/method blocks, so the indentation levels are not used for the sake of
# backtracking at the moment.

class A # error: Hint: this "class" token is not closed before the end of the file
  def foo(x, y) # error: Hint: this "def" token is not closed before the end of the file
    unless x # error: Hint: this "unless" token is not closed before the end of the file
    while x # error: Hint: this "while" token is not closed before the end of the file
    until x # error: Hint: this "until" token is not closed before the end of the file
    1.times do # error: Hint: this "do" token is not closed before the end of the file
    -> do # error: Hint: this kDO_LAMBDA token is not closed before the end of the file
    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
    # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (9)</summary>

```
test/testdata/parser/error_recovery/eof_8.rb:16: unexpected token "end of file" https://srb.help/2001
    16 |    # error: unexpected token "end of file"
    17 |

test/testdata/parser/error_recovery/eof_8.rb:15: Hint: this kDO_BLOCK token is not closed before the end of the file https://srb.help/2003
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                ^^

test/testdata/parser/error_recovery/eof_8.rb:14: Hint: this kDO_LAMBDA token is not closed before the end of the file https://srb.help/2003
    14 |    -> do # error: Hint: this kDO_LAMBDA token is not closed before the end of the file
               ^^

test/testdata/parser/error_recovery/eof_8.rb:13: Hint: this "do" token is not closed before the end of the file https://srb.help/2003
    13 |    1.times do # error: Hint: this "do" token is not closed before the end of the file
                    ^^

test/testdata/parser/error_recovery/eof_8.rb:12: Hint: this "until" token is not closed before the end of the file https://srb.help/2003
    12 |    until x # error: Hint: this "until" token is not closed before the end of the file
            ^^^^^

test/testdata/parser/error_recovery/eof_8.rb:11: Hint: this "while" token is not closed before the end of the file https://srb.help/2003
    11 |    while x # error: Hint: this "while" token is not closed before the end of the file
            ^^^^^

test/testdata/parser/error_recovery/eof_8.rb:10: Hint: this "unless" token is not closed before the end of the file https://srb.help/2003
    10 |    unless x # error: Hint: this "unless" token is not closed before the end of the file
            ^^^^^^

test/testdata/parser/error_recovery/eof_8.rb:9: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     9 |  def foo(x, y) # error: Hint: this "def" token is not closed before the end of the file
          ^^^

test/testdata/parser/error_recovery/eof_8.rb:8: Hint: this "class" token is not closed before the end of the file https://srb.help/2003
     8 |class A # error: Hint: this "class" token is not closed before the end of the file
        ^^^^^
Errors: 9
```

</details>

<details>
<summary>Prism errors (8)</summary>

```
test/testdata/parser/error_recovery/eof_8.rb:15: expected a block beginning with `do` to end with `end` https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected a lambda block beginning with `do` to end with `end` https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected a block beginning with `do` to end with `end` https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected an `end` to close the `until` statement https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected an `end` to close the `while` statement https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected an `end` to close the conditional clause https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected an `end` to close the `def` statement https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^

test/testdata/parser/error_recovery/eof_8.rb:15: expected an `end` to close the `class` statement https://srb.help/2001
    15 |    x y do # error: Hint: this kDO_BLOCK token is not closed before the end of the file
                  ^
Errors: 8
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism reports all 8 errors at line 15 (the last line) instead of pointing to each unclosed construct's actual location, making it much harder to identify which tokens need closing.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, y, &<blk>)
      if x
        <emptyTree>
      else
        while x
          while x.!()
            1.times() do ||
              <emptyTree>::<C Kernel>.lambda() do ||
                <self>.x(y) do ||
                  <emptyTree>
                end
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(x, y, &<blk>)
      if x
        <emptyTree>
      else
        while x
          while x.!()
            1.times() do ||
              <emptyTree>::<C Kernel>.lambda() do ||
                <self>.x(y) do ||
                  <emptyTree>::<C <ErrorNode>>
                end
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -9,7 +9,7 @@
             1.times() do ||
               <emptyTree>::<C Kernel>.lambda() do ||
                 <self>.x(y) do ||
-                  <emptyTree>
+                  <emptyTree>::<C <ErrorNode>>
                 end
               end
             end
```

</details>

## eof_9.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

case x # error: Hint: this "case" token is not closed before the end of the file
when true

case # error: Hint: this "case" token is not closed before the end of the file
when x

  case x # error: Hint: this "case" token is not closed before the end of the file
# ^^^^ error: unexpected token "case"


```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_9.rb:9: unexpected token "case" https://srb.help/2001
     9 |  case x # error: Hint: this "case" token is not closed before the end of the file
          ^^^^

test/testdata/parser/error_recovery/eof_9.rb:9: Hint: this "case" token is not closed before the end of the file https://srb.help/2003
     9 |  case x # error: Hint: this "case" token is not closed before the end of the file
          ^^^^

test/testdata/parser/error_recovery/eof_9.rb:6: Hint: this "case" token is not closed before the end of the file https://srb.help/2003
     6 |case # error: Hint: this "case" token is not closed before the end of the file
        ^^^^

test/testdata/parser/error_recovery/eof_9.rb:3: Hint: this "case" token is not closed before the end of the file https://srb.help/2003
     3 |case x # error: Hint: this "case" token is not closed before the end of the file
        ^^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/eof_9.rb:9: expected a `when` or `in` clause after `case` https://srb.help/2001
     9 |  case x # error: Hint: this "case" token is not closed before the end of the file
          ^^^^

test/testdata/parser/error_recovery/eof_9.rb:10: expected an `end` to close the `case` statement https://srb.help/2001
    10 |# ^^^^ error: unexpected token "case"
        ^

test/testdata/parser/error_recovery/eof_9.rb:12: expected an `end` to close the `case` statement https://srb.help/2001
    12 |
        ^

test/testdata/parser/error_recovery/eof_9.rb:12: expected an `end` to close the `case` statement https://srb.help/2001
    12 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the critical nested case error on line 9 and points to EOF/comments instead of the actual problem locations, making errors less actionable than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <assignTemp>$2 = <self>.x()

  if true.===(<assignTemp>$2)
    if <self>.x()
      begin
        <assignTemp>$3 = <self>.x()
        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$3)
          <emptyTree>
        else
          <emptyTree>
        end
      end
    else
      <emptyTree>
    end
  else
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <assignTemp>$2 = <self>.x()

  if true.===(<assignTemp>$2)
    if <self>.x()
      begin
        begin
          <assignTemp>$2 = <self>.x()
          <emptyTree>
        end
        <emptyTree>::<C <ErrorNode>>
      end
    else
      <emptyTree>
    end
  else
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,12 +4,11 @@
   if true.===(<assignTemp>$2)
     if <self>.x()
       begin
-        <assignTemp>$3 = <self>.x()
-        if <emptyTree>::<C <ErrorNode>>.===(<assignTemp>$3)
+        begin
+          <assignTemp>$2 = <self>.x()
           <emptyTree>
-        else
-          <emptyTree>
         end
+        <emptyTree>::<C <ErrorNode>>
       end
     else
       <emptyTree>
```

</details>

## forward_args.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

def foo(*, ...)
      # ^ error: ... after rest argument
  [1,2,3].each { |x| _1 }
                   # ^^ error: numbered parameters are not allowed when an ordinary parameter is defined
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/forward_args.rb:3: ... after rest argument https://srb.help/2001
     3 |def foo(*, ...)
                ^

test/testdata/parser/error_recovery/forward_args.rb:5: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     5 |  [1,2,3].each { |x| _1 }
                             ^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/forward_args.rb:3: ... after rest argument https://srb.help/2001
     3 |def foo(*, ...)
                   ^^^

test/testdata/parser/error_recovery/forward_args.rb:5: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     5 |  [1,2,3].each { |x| _1 }
                             ^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism errors are accurate and slightly better, pointing to the full ... token instead of just the first character, while catching both issues correctly.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(**, *<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
    [1, 2, 3].each() do |_1|
      _1
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(**, *<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
    [1, 2, 3].each() do |x|
      _1
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,6 +1,6 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def foo<<todo method>>(**, *<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
-    [1, 2, 3].each() do |_1|
+    [1, 2, 3].each() do |x|
       _1
     end
   end
```

</details>

## forward_args_with_block.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

def test(...)
  [1,2,3].each do
    foo(...) do end
  #     ^^^         error: both block argument and literal block are passed
  end
end

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/forward_args_with_block.rb:5: both block argument and literal block are passed https://srb.help/2001
     5 |    foo(...) do end
                ^^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/forward_args_with_block.rb:5: both block arg and actual block given; only one block is allowed https://srb.help/2001
     5 |    foo(...) do end
                     ^^^^^^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same error with a clearer message, but points to the entire block instead of the forwarding arguments which is less precise than Original's location.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test<<todo method>>(*<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
    [1, 2, 3].each() do ||
      ::<Magic>.<call-with-splat-and-block-pass>(<self>, :foo, ::<Magic>.<splat>(<fwd-args>).concat([::<Magic>.<to-hash-dup>(<fwd-kwargs>)]), nil, <fwd-block>) do ||
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test<<todo method>>(*<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
    [1, 2, 3].each() do ||
      ::<Magic>.<call-with-splat-and-block-pass>(<self>, :foo, ::<Magic>.<splat>(<fwd-args>).concat([::<Magic>.<to-hash-dup>(<fwd-kwargs>)]), nil, <fwd-block>)
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,9 +1,7 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def test<<todo method>>(*<fwd-args>, *<fwd-kwargs>:, &<fwd-block>)
     [1, 2, 3].each() do ||
-      ::<Magic>.<call-with-splat-and-block-pass>(<self>, :foo, ::<Magic>.<splat>(<fwd-args>).concat([::<Magic>.<to-hash-dup>(<fwd-kwargs>)]), nil, <fwd-block>) do ||
-        <emptyTree>
-      end
+      ::<Magic>.<call-with-splat-and-block-pass>(<self>, :foo, ::<Magic>.<splat>(<fwd-args>).concat([::<Magic>.<to-hash-dup>(<fwd-kwargs>)]), nil, <fwd-block>)
     end
   end
 end
```

</details>

## hash_pair_value_omission.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

[1,2,3].map do |x|
  { foo?: }
  # ^^^^^ error: Method `foo?` does not exist
  # ^^^^^ error: identifier foo? is not valid to get
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/hash_pair_value_omission.rb:4: identifier foo? is not valid to get https://srb.help/2001
     4 |  { foo?: }
            ^^^^^

test/testdata/parser/error_recovery/hash_pair_value_omission.rb:4: Method `foo?` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  { foo?: }
            ^^^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/hash_pair_value_omission.rb:3:
     3 |[1,2,3].map do |x|
        ^
    https://github.com/sorbet/sorbet/tree/master/rbi/core/kernel.rbi#L473: Did you mean: `Kernel#fork`
     473 |  def fork(&blk); end
            ^^^^^^^^^^^^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/hash_pair_value_omission.rb:4: identifier foo? is not valid to get https://srb.help/2001
     4 |  { foo?: }
            ^^^^

test/testdata/parser/error_recovery/hash_pair_value_omission.rb:4: Method `foo?` does not exist on `T.class_of(<root>)` https://srb.help/7003
     4 |  { foo?: }
            ^^^^^
  Got `T.class_of(<root>)` originating from:
    test/testdata/parser/error_recovery/hash_pair_value_omission.rb:3:
     3 |[1,2,3].map do |x|
        ^
    https://github.com/sorbet/sorbet/tree/master/rbi/core/kernel.rbi#L473: Did you mean: `Kernel#fork`
     473 |  def fork(&blk); end
            ^^^^^^^^^^^^^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same errors but underlines one fewer character in the first error (4 carets vs 5), a minor cosmetic difference in error location precision.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |x|
    {:foo? => <self>.foo?()}
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |x|
    {:foo? => <self>.foo?()}
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## if_do_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

class A
  if 'thing' do
# ^^ error: Unexpected token "if"; did you mean "it"?
  end
end

```

</details>

<details>
<summary>Original errors (1) | Autocorrects: 1</summary>

```
test/testdata/parser/error_recovery/if_do_1.rb:4: Unexpected token "if"; did you mean "it"? https://srb.help/2001
     4 |  if 'thing' do
          ^^
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/if_do_1.rb:4: Replace with `it`
     4 |  if 'thing' do
          ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/if_do_1.rb:4: expected `then` or `;` or '\n' https://srb.help/2001
     4 |  if 'thing' do
                     ^^

test/testdata/parser/error_recovery/if_do_1.rb:4: unexpected 'do', ignoring it https://srb.help/2001
     4 |  if 'thing' do
                     ^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism correctly identifies the syntax error with accurate location pointing to the problematic 'do' keyword, providing clear technical error messages about the missing separator in the if statement.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    if "thing"
      <emptyTree>
    else
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    if "thing"
      <emptyTree>::<C <ErrorNode>>
    else
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,7 +1,7 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     if "thing"
-      <emptyTree>
+      <emptyTree>::<C <ErrorNode>>
     else
       <emptyTree>
     end
```

</details>

## if_do_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    puts 'before'
    # TODO(jez) Better error would be to point to the `do` keyword here
    if x.y do # error: Hint: this "if" token might not be properly closed
      puts 'then'
    end
    Integer.class
  end

  def test2
    puts 'before'
    # TODO(jez) Better error would be to point to the `do` keyword here
    if x.y do # error: Hint: this "if" token might not be properly closed
      puts 'then'
    else # error: else without rescue is useless
      # TODO(jez) better parse would drop the `do` keyword and put this branch in the else
      puts 'else'
    end
    Integer.class
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/if_do_2.rb:18: else without rescue is useless https://srb.help/2001
    18 |    else # error: else without rescue is useless
            ^^^^

test/testdata/parser/error_recovery/if_do_2.rb:24: unexpected token "end of file" https://srb.help/2001
    24 |end # error: unexpected token "end of file"
    25 |

test/testdata/parser/error_recovery/if_do_2.rb:7: Hint: this "if" token might not be properly closed https://srb.help/2003
     7 |    if x.y do # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_do_2.rb:11: Matching `end` found here but is not indented as far
    11 |  end
          ^^^

test/testdata/parser/error_recovery/if_do_2.rb:16: Hint: this "if" token might not be properly closed https://srb.help/2003
    16 |    if x.y do # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_do_2.rb:23: Matching `end` found here but is not indented as far
    23 |  end
          ^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/if_do_2.rb:18: unexpected 'else', ignoring it https://srb.help/2001
    18 |    else # error: else without rescue is useless
            ^^^^

test/testdata/parser/error_recovery/if_do_2.rb:25: expected an `end` to close the `def` statement https://srb.help/2001
    25 |
        ^

test/testdata/parser/error_recovery/if_do_2.rb:25: expected an `end` to close the `class` statement https://srb.help/2001
    25 |
        ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the key diagnostic hints about unclosed if statements and points to EOF instead of the actual problematic do keywords, making it harder to identify the root cause.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <self>.x().y() do ||
            <self>.puts("then")
          end
          <emptyTree>::<C Integer>.class()
        else
          <emptyTree>
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <self>.x().y() do ||
            begin
              <self>.puts("then")
              <self>.puts("else")
            end
          end
          <emptyTree>::<C Integer>.class()
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <self>.x().y() do ||
            <self>.puts("then")
          end
          <emptyTree>::<C Integer>.class()
        else
          <emptyTree>
        end
        def test2<<todo method>>(&<blk>)
          begin
            <self>.puts("before")
            if <self>.x().y() do ||
                begin
                  <self>.puts("then")
                  <emptyTree>::<C <ErrorNode>>
                  <self>.puts("else")
                end
              end
              <emptyTree>::<C Integer>.class()
            else
              <emptyTree>
            end
          end
        end
        <emptyTree>::<C <ErrorNode>>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -10,22 +10,23 @@
         else
           <emptyTree>
         end
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        if <self>.x().y() do ||
-            begin
-              <self>.puts("then")
-              <self>.puts("else")
+        def test2<<todo method>>(&<blk>)
+          begin
+            <self>.puts("before")
+            if <self>.x().y() do ||
+                begin
+                  <self>.puts("then")
+                  <emptyTree>::<C <ErrorNode>>
+                  <self>.puts("else")
+                end
+              end
+              <emptyTree>::<C Integer>.class()
+            else
+              <emptyTree>
             end
           end
-          <emptyTree>::<C Integer>.class()
-        else
-          <emptyTree>
         end
+        <emptyTree>::<C <ErrorNode>>
       end
     end
   end
```

</details>

## if_indent_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test0
    x = nil
    if # error: unexpected token "if"
  end

  def test1
    x = nil
    if x # error: Hint: this "if" token might not be properly closed
  end

  def test2
    x = nil
    if x. # error: Hint: this "if" token might not be properly closed
  end
# ^^^ error: unexpected token "end"

  def test3
    x = nil
    if x.f # error: Hint: this "if" token might not be properly closed
  end

  # -- These should still have no errors even in indentationAware mode --

  def no_syntax_error_1
    x = if y
    end
  end

  def no_syntax_error_2
    x = if y
        end
  end

  def no_syntax_error_3
      # this is a comment with weird indent
    x = if y
    end
    puts 'after'
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (6)</summary>

```
test/testdata/parser/error_recovery/if_indent_1.rb:6: unexpected token "if" https://srb.help/2001
     6 |    if # error: unexpected token "if"
            ^^

test/testdata/parser/error_recovery/if_indent_1.rb:17: unexpected token "end" https://srb.help/2001
    17 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_1.rb:43: unexpected token "end of file" https://srb.help/2001
    43 |end # error: unexpected token "end of file"
    44 |

test/testdata/parser/error_recovery/if_indent_1.rb:11: Hint: this "if" token might not be properly closed https://srb.help/2003
    11 |    if x # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_1.rb:12: Matching `end` found here but is not indented as far
    12 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_1.rb:16: Hint: this "if" token might not be properly closed https://srb.help/2003
    16 |    if x. # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_1.rb:17: Matching `end` found here but is not indented as far
    17 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_1.rb:22: Hint: this "if" token might not be properly closed https://srb.help/2003
    22 |    if x.f # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_1.rb:23: Matching `end` found here but is not indented as far
    23 |  end
          ^^^
Errors: 6
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/if_indent_1.rb:6: expected a predicate expression for the `if` statement https://srb.help/2001
     6 |    if # error: unexpected token "if"
            ^^

test/testdata/parser/error_recovery/if_indent_1.rb:7: expected `then` or `;` or '\n' https://srb.help/2001
     7 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_1.rb:7: expected an `end` to close the `def` statement https://srb.help/2001
     7 |  end
             ^

test/testdata/parser/error_recovery/if_indent_1.rb:7: expected an `end` to close the `class` statement https://srb.help/2001
     7 |  end
             ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism only catches the first syntax error and cascades incorrectly, missing all the indentation-based hints for test1, test2, and test3 that Original properly detects.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      begin
        x = nil
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test1<<todo method>>(&<blk>)
      begin
        x = nil
        if x
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        x = nil
        if x.<method-name-missing>()
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test3<<todo method>>(&<blk>)
      begin
        x = nil
        if x.f()
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def no_syntax_error_1<<todo method>>(&<blk>)
      x = if <self>.y()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def no_syntax_error_2<<todo method>>(&<blk>)
      x = if <self>.y()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def no_syntax_error_3<<todo method>>(&<blk>)
      begin
        x = if <self>.y()
          <emptyTree>
        else
          <emptyTree>
        end
        <self>.puts("after")
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      begin
        x = nil
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -10,65 +10,5 @@
         end
       end
     end
-
-    def test1<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x.<method-name-missing>()
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def test3<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x.f()
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def no_syntax_error_1<<todo method>>(&<blk>)
-      x = if <self>.y()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
-
-    def no_syntax_error_2<<todo method>>(&<blk>)
-      x = if <self>.y()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
-
-    def no_syntax_error_3<<todo method>>(&<blk>)
-      begin
-        x = if <self>.y()
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-        <self>.puts("after")
-      end
-    end
   end
 end
```

</details>

## if_indent_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test0
    x = nil
    if # error: Hint: this "if" token might not be properly closed
    puts 'after'
  end

  def test1
    x = nil
    if x # error: Hint: this "if" token might not be properly closed
    puts 'after'
  end

  def test2
    x = nil
    if x. # error: Hint: this "if" token might not be properly closed
    puts 'after'
  end

  def test3
    x = nil
    if x.f # error: Hint: this "if" token might not be properly closed
    puts 'after'
  end

  def test4
    x = nil
    if x.f() # error: Hint: this "if" token might not be properly closed
    puts 'after'
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (6)</summary>

```
test/testdata/parser/error_recovery/if_indent_2.rb:33: unexpected token "end of file" https://srb.help/2001
    33 |end # error: unexpected token "end of file"
    34 |

test/testdata/parser/error_recovery/if_indent_2.rb:6: Hint: this "if" token might not be properly closed https://srb.help/2003
     6 |    if # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_2.rb:8: Matching `end` found here but is not indented as far
     8 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_2.rb:12: Hint: this "if" token might not be properly closed https://srb.help/2003
    12 |    if x # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_2.rb:14: Matching `end` found here but is not indented as far
    14 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_2.rb:18: Hint: this "if" token might not be properly closed https://srb.help/2003
    18 |    if x. # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_2.rb:20: Matching `end` found here but is not indented as far
    20 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_2.rb:24: Hint: this "if" token might not be properly closed https://srb.help/2003
    24 |    if x.f # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_2.rb:26: Matching `end` found here but is not indented as far
    26 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_2.rb:30: Hint: this "if" token might not be properly closed https://srb.help/2003
    30 |    if x.f() # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_2.rb:32: Matching `end` found here but is not indented as far
    32 |  end
          ^^^
Errors: 6
```

</details>

<details>
<summary>Prism errors (5)</summary>

```
test/testdata/parser/error_recovery/if_indent_2.rb:34: expected an `end` to close the `def` statement https://srb.help/2001
    34 |
        ^

test/testdata/parser/error_recovery/if_indent_2.rb:34: expected an `end` to close the `def` statement https://srb.help/2001
    34 |
        ^

test/testdata/parser/error_recovery/if_indent_2.rb:34: expected an `end` to close the `def` statement https://srb.help/2001
    34 |
        ^

test/testdata/parser/error_recovery/if_indent_2.rb:34: expected an `end` to close the `def` statement https://srb.help/2001
    34 |
        ^

test/testdata/parser/error_recovery/if_indent_2.rb:34: expected an `end` to close the `class` statement https://srb.help/2001
    34 |
        ^
Errors: 5
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the core indentation mismatch issues that Original catches and provides less helpful generic EOF errors instead of pinpointing the specific if-end indentation problems.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      begin
        x = nil
        if <self>.puts("after")
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test1<<todo method>>(&<blk>)
      begin
        x = nil
        if x
          <self>.puts("after")
        else
          <emptyTree>
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        x = nil
        if x.puts("after")
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test3<<todo method>>(&<blk>)
      begin
        x = nil
        if x.f()
          <self>.puts("after")
        else
          <emptyTree>
        end
      end
    end

    def test4<<todo method>>(&<blk>)
      begin
        x = nil
        if x.f()
          <self>.puts("after")
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      begin
        x = nil
        if <self>.puts("after")
          <emptyTree>
        else
          <emptyTree>
        end
        def test1<<todo method>>(&<blk>)
          begin
            x = nil
            if x
              <self>.puts("after")
            else
              <emptyTree>
            end
            def test2<<todo method>>(&<blk>)
              begin
                x = nil
                if x.puts("after")
                  <emptyTree>
                else
                  <emptyTree>
                end
                def test3<<todo method>>(&<blk>)
                  begin
                    x = nil
                    if x.f()
                      <self>.puts("after")
                    else
                      <emptyTree>
                    end
                    def test4<<todo method>>(&<blk>)
                      begin
                        x = nil
                        if x.f()
                          <self>.puts("after")
                        else
                          <emptyTree>
                        end
                      end
                    end
                    <emptyTree>::<C <ErrorNode>>
                  end
                end
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -8,51 +8,48 @@
         else
           <emptyTree>
         end
-      end
-    end
-
-    def test1<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x
-          <self>.puts("after")
-        else
-          <emptyTree>
+        def test1<<todo method>>(&<blk>)
+          begin
+            x = nil
+            if x
+              <self>.puts("after")
+            else
+              <emptyTree>
+            end
+            def test2<<todo method>>(&<blk>)
+              begin
+                x = nil
+                if x.puts("after")
+                  <emptyTree>
+                else
+                  <emptyTree>
+                end
+                def test3<<todo method>>(&<blk>)
+                  begin
+                    x = nil
+                    if x.f()
+                      <self>.puts("after")
+                    else
+                      <emptyTree>
+                    end
+                    def test4<<todo method>>(&<blk>)
+                      begin
+                        x = nil
+                        if x.f()
+                          <self>.puts("after")
+                        else
+                          <emptyTree>
+                        end
+                      end
+                    end
+                    <emptyTree>::<C <ErrorNode>>
+                  end
+                end
+              end
+            end
+          end
         end
       end
     end
-
-    def test2<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x.puts("after")
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def test3<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x.f()
-          <self>.puts("after")
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def test4<<todo method>>(&<blk>)
-      begin
-        x = nil
-        if x.f()
-          <self>.puts("after")
-        else
-          <emptyTree>
-        end
-      end
-    end
   end
 end
```

</details>

## if_indent_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test0
    x = if # error: unexpected token "if"
  end

  def test1
    x = if x # error: Hint: this "if" token might not be properly closed
  end

  def test2
    x = if x. # error: Hint: this "if" token might not be properly closed
  end
# ^^^ error: unexpected token "end"

  def test3
    x = if x.f # error: Hint: this "if" token might not be properly closed
  end

  def test3
    x = if x.f() # error: Hint: this "if" token might not be properly closed
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (7)</summary>

```
test/testdata/parser/error_recovery/if_indent_3.rb:5: unexpected token "if" https://srb.help/2001
     5 |    x = if # error: unexpected token "if"
                ^^

test/testdata/parser/error_recovery/if_indent_3.rb:14: unexpected token "end" https://srb.help/2001
    14 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_3.rb:24: unexpected token "end of file" https://srb.help/2001
    24 |end # error: unexpected token "end of file"
    25 |

test/testdata/parser/error_recovery/if_indent_3.rb:9: Hint: this "if" token might not be properly closed https://srb.help/2003
     9 |    x = if x # error: Hint: this "if" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/if_indent_3.rb:10: Matching `end` found here but is not indented as far
    10 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_3.rb:13: Hint: this "if" token might not be properly closed https://srb.help/2003
    13 |    x = if x. # error: Hint: this "if" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/if_indent_3.rb:14: Matching `end` found here but is not indented as far
    14 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_3.rb:18: Hint: this "if" token might not be properly closed https://srb.help/2003
    18 |    x = if x.f # error: Hint: this "if" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/if_indent_3.rb:19: Matching `end` found here but is not indented as far
    19 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_3.rb:22: Hint: this "if" token might not be properly closed https://srb.help/2003
    22 |    x = if x.f() # error: Hint: this "if" token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/if_indent_3.rb:23: Matching `end` found here but is not indented as far
    23 |  end
          ^^^
Errors: 7
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/if_indent_3.rb:5: expected a predicate expression for the `if` statement https://srb.help/2001
     5 |    x = if # error: unexpected token "if"
                ^^

test/testdata/parser/error_recovery/if_indent_3.rb:6: expected `then` or `;` or '\n' https://srb.help/2001
     6 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_3.rb:6: expected an `end` to close the `def` statement https://srb.help/2001
     6 |  end
             ^

test/testdata/parser/error_recovery/if_indent_3.rb:6: expected an `end` to close the `class` statement https://srb.help/2001
     6 |  end
             ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism only catches the first error and cascades into incorrect recovery, missing 6 out of 7 errors that Original correctly identifies including all the helpful indentation hints for unclosed if statements.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      x = if <emptyTree>::<C <ErrorNode>>
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test1<<todo method>>(&<blk>)
      x = if x
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test2<<todo method>>(&<blk>)
      x = if x.<method-name-missing>()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test3<<todo method>>(&<blk>)
      x = if x.f()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    def test3<<todo method>>(&<blk>)
      x = if x.f()
        <emptyTree>
      else
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test0<<todo method>>(&<blk>)
      x = if <emptyTree>::<C <ErrorNode>>
        <emptyTree>
      else
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -7,37 +7,5 @@
         <emptyTree>
       end
     end
-
-    def test1<<todo method>>(&<blk>)
-      x = if x
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      x = if x.<method-name-missing>()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
-
-    def test3<<todo method>>(&<blk>)
-      x = if x.f()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
-
-    def test3<<todo method>>(&<blk>)
-      x = if x.f()
-        <emptyTree>
-      else
-        <emptyTree>
-      end
-    end
   end
 end
```

</details>

## if_indent_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  # This method actually parses normally, but because there was a syntax error
  # *somewhere* in the file, we eagerly report an error here attempting to
  # recover from it.
  def test1
    if x.f # error: Hint: this "if" token might not be properly closed
    puts 'inside if'
  end
    # We this ends up at the class top-level, not inside test1
    puts 'after if but inside test1'
  end

  # This ends up outside of `A`, but maybe it's better than showing nothing in
  # the whole file.
  def test2
    if x.f # error: Hint: this "if" token might not be properly closed
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/if_indent_4.rb:20: unexpected token "end of file" https://srb.help/2001
    20 |end # error: unexpected token "end of file"
    21 |

test/testdata/parser/error_recovery/if_indent_4.rb:8: Hint: this "if" token might not be properly closed https://srb.help/2003
     8 |    if x.f # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_4.rb:10: Matching `end` found here but is not indented as far
    10 |  end
          ^^^

test/testdata/parser/error_recovery/if_indent_4.rb:18: Hint: this "if" token might not be properly closed https://srb.help/2003
    18 |    if x.f # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_indent_4.rb:19: Matching `end` found here but is not indented as far
    19 |  end
          ^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/if_indent_4.rb:21: expected an `end` to close the `class` statement https://srb.help/2001
    21 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism only reports a generic missing end error at EOF while Original correctly identifies both problematic if statements with indentation mismatches at their actual locations

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      if <self>.x().f()
        <self>.puts("inside if")
      else
        <emptyTree>
      end
    end

    <self>.puts("after if but inside test1")
  end

  def test2<<todo method>>(&<blk>)
    if <self>.x().f()
      <emptyTree>
    else
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        if <self>.x().f()
          <self>.puts("inside if")
        else
          <emptyTree>
        end
        <self>.puts("after if but inside test1")
      end
    end

    def test2<<todo method>>(&<blk>)
      if <self>.x().f()
        <emptyTree>
      else
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,21 +1,24 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def test1<<todo method>>(&<blk>)
+      begin
+        if <self>.x().f()
+          <self>.puts("inside if")
+        else
+          <emptyTree>
+        end
+        <self>.puts("after if but inside test1")
+      end
+    end
+
+    def test2<<todo method>>(&<blk>)
       if <self>.x().f()
-        <self>.puts("inside if")
+        <emptyTree>
       else
         <emptyTree>
       end
     end
 
-    <self>.puts("after if but inside test1")
+    <emptyTree>::<C <ErrorNode>>
   end
-
-  def test2<<todo method>>(&<blk>)
-    if <self>.x().f()
-      <emptyTree>
-    else
-      <emptyTree>
-    end
-  end
 end
```

</details>

## if_indent_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module A::B
  class C1
    def foo
      if # error: unexpected token "if"
    end

    sig {void}
    private def bar; end
  end

  class C2
    def foo
      if x # error: Hint: this "if" token might not be properly closed
    end

    sig {void}
    private def bar; end
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/if_indent_5.rb:6: unexpected token "if" https://srb.help/2001
     6 |      if # error: unexpected token "if"
              ^^

test/testdata/parser/error_recovery/if_indent_5.rb:21: unexpected token "end of file" https://srb.help/2001
    21 |end # error: unexpected token "end of file"
    22 |

test/testdata/parser/error_recovery/if_indent_5.rb:15: Hint: this "if" token might not be properly closed https://srb.help/2003
    15 |      if x # error: Hint: this "if" token might not be properly closed
              ^^
    test/testdata/parser/error_recovery/if_indent_5.rb:16: Matching `end` found here but is not indented as far
    16 |    end
            ^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (5)</summary>

```
test/testdata/parser/error_recovery/if_indent_5.rb:6: expected a predicate expression for the `if` statement https://srb.help/2001
     6 |      if # error: unexpected token "if"
              ^^

test/testdata/parser/error_recovery/if_indent_5.rb:7: expected `then` or `;` or '\n' https://srb.help/2001
     7 |    end
            ^^^

test/testdata/parser/error_recovery/if_indent_5.rb:7: expected an `end` to close the `def` statement https://srb.help/2001
     7 |    end
               ^

test/testdata/parser/error_recovery/if_indent_5.rb:7: expected an `end` to close the `class` statement https://srb.help/2001
     7 |    end
               ^

test/testdata/parser/error_recovery/if_indent_5.rb:7: expected an `end` to close the `module` statement https://srb.help/2001
     7 |    end
               ^
Errors: 5
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism generates a cascade of 5 confusing errors all pointing to line 7 for the first malformed if statement, while missing the second malformed if statement on line 15 entirely that Original correctly identifies with a helpful indentation hint.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A>::<C B><<C <todo sym>>> < ()
    class <emptyTree>::<C C1><<C <todo sym>>> < (::<todo sym>)
      def foo<<todo method>>(&<blk>)
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end

      <self>.sig() do ||
        <self>.void()
      end

      <self>.private(def bar<<todo method>>(&<blk>)
          <emptyTree>
        end)
    end

    class <emptyTree>::<C C2><<C <todo sym>>> < (::<todo sym>)
      def foo<<todo method>>(&<blk>)
        if <self>.x()
          <emptyTree>
        else
          <emptyTree>
        end
      end

      <self>.sig() do ||
        <self>.void()
      end

      <self>.private(def bar<<todo method>>(&<blk>)
          <emptyTree>
        end)
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C A>::<C B><<C <todo sym>>> < ()
    class <emptyTree>::<C C1><<C <todo sym>>> < (::<todo sym>)
      def foo<<todo method>>(&<blk>)
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -8,32 +8,6 @@
           <emptyTree>
         end
       end
-
-      <self>.sig() do ||
-        <self>.void()
-      end
-
-      <self>.private(def bar<<todo method>>(&<blk>)
-          <emptyTree>
-        end)
     end
-
-    class <emptyTree>::<C C2><<C <todo sym>>> < (::<todo sym>)
-      def foo<<todo method>>(&<blk>)
-        if <self>.x()
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-
-      <self>.sig() do ||
-        <self>.void()
-      end
-
-      <self>.private(def bar<<todo method>>(&<blk>)
-          <emptyTree>
-        end)
-    end
   end
 end
```

</details>

## if_no_end.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def foo
    if true # error: Hint: this "if" token might not be properly closed
  end
end

class B
  def bar
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/if_no_end.rb:12: unexpected token "end of file" https://srb.help/2001
    12 |end # error: unexpected token "end of file"
    13 |

test/testdata/parser/error_recovery/if_no_end.rb:5: Hint: this "if" token might not be properly closed https://srb.help/2003
     5 |    if true # error: Hint: this "if" token might not be properly closed
            ^^
    test/testdata/parser/error_recovery/if_no_end.rb:6: Matching `end` found here but is not indented as far
     6 |  end
          ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/if_no_end.rb:13: expected an `end` to close the `class` statement https://srb.help/2001
    13 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (unclosed if statement on line 5) and only reports a generic class end error, while Original correctly identifies both the unclosed if and provides helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(&<blk>)
      if true
        <emptyTree>
      else
        <emptyTree>
      end
    end
  end

  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def bar<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(&<blk>)
      if true
        <emptyTree>
      else
        <emptyTree>
      end
    end

    class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
      def bar<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -7,11 +7,13 @@
         <emptyTree>
       end
     end
-  end
 
-  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
-    def bar<<todo method>>(&<blk>)
-      <emptyTree>
+    class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
+      def bar<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_01.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A # error: class definition in method body
  sig {void}
  def
# ^^^ error: Hint: this "def" token might not be followed by a method name
# ^^^ error: Hint: this "def" token might not be properly closed

  sig {void} # error: unexpected token tLCURLY
  def example; end
end

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_01.rb:8: unexpected token tLCURLY https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
              ^

test/testdata/parser/error_recovery/lonely_def_01.rb:2: class definition in method body https://srb.help/2001
     2 |class A # error: class definition in method body
        ^^^^^

test/testdata/parser/error_recovery/lonely_def_01.rb:4: Hint: this "def" token might not be followed by a method name https://srb.help/2003
     4 |  def
          ^^^

test/testdata/parser/error_recovery/lonely_def_01.rb:4: Hint: this "def" token might not be properly closed https://srb.help/2003
     4 |  def
          ^^^
    test/testdata/parser/error_recovery/lonely_def_01.rb:10: Matching `end` found here but is not indented as far
    10 |end
        ^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_01.rb:8: expected a delimiter to close the parameters https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
             ^

test/testdata/parser/error_recovery/lonely_def_01.rb:8: expected a `=>` between the hash key and value https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
                   ^

test/testdata/parser/error_recovery/lonely_def_01.rb:8: unexpected '}'; expected a value in the hash literal https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
                   ^

test/testdata/parser/error_recovery/lonely_def_01.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism completely misses the core issue of the lonely def without a method name and instead produces confusing hash-related errors that don't reflect the actual problem.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def <method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def example<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def sig<<todo method>>(&<blk>)
      begin
        {<self>.void() => <emptyTree>::<C <ErrorNode>>}
        def example<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,16 +4,15 @@
       <self>.void()
     end
 
-    def <method-def-name-missing><<todo method>>(&<blk>)
-      <emptyTree>
+    def sig<<todo method>>(&<blk>)
+      begin
+        {<self>.void() => <emptyTree>::<C <ErrorNode>>}
+        def example<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def example<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_02.rb

<details>
<summary>Input</summary>

```ruby
# typed: true
class A
  extend T::Sig
  sig {params(x: Integer).void}
  #           ^ error: Unknown parameter name `x`
  def
# ^^^ error: Hint: this "def" token might not be followed by a method name
# ^^^ error: Hint: this "def" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_02.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/lonely_def_02.rb:6: Hint: this "def" token might not be followed by a method name https://srb.help/2003
     6 |  def
          ^^^

test/testdata/parser/error_recovery/lonely_def_02.rb:6: Hint: this "def" token might not be properly closed https://srb.help/2003
     6 |  def
          ^^^
    test/testdata/parser/error_recovery/lonely_def_02.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^

test/testdata/parser/error_recovery/lonely_def_02.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_02.rb:6: Parameter not in method definition here
     6 |  def
          ^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/lonely_def_02.rb:10: expected an `end` to close the `def` statement https://srb.help/2001
    10 |
        ^

test/testdata/parser/error_recovery/lonely_def_02.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^

test/testdata/parser/error_recovery/lonely_def_02.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_02.rb:6: Parameter not in method definition here
     6 |  def
     7 |# ^^^ error: Hint: this "def" token might not be followed by a method name
     8 |# ^^^ error: Hint: this "def" token might not be properly closed
     9 |end # error: unexpected token "end of file"
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same issues but points to EOF instead of the problematic def token and end token, making error locations less precise than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def <method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def end<<todo method>>(&<blk>)
      <emptyTree>::<C <ErrorNode>>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -6,8 +6,8 @@
       <self>.params(:x, <emptyTree>::<C Integer>).void()
     end
 
-    def <method-def-name-missing><<todo method>>(&<blk>)
-      <emptyTree>
+    def end<<todo method>>(&<blk>)
+      <emptyTree>::<C <ErrorNode>>
     end
   end
 end
```

</details>

## lonely_def_03.rb

<details>
<summary>Input</summary>

```ruby
# typed: true
def # error: Hint: this "def" token is not closed before the end of the file
# error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_03.rb:3: unexpected token "end of file" https://srb.help/2001
     3 |# error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_03.rb:2: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     2 |def # error: Hint: this "def" token is not closed before the end of the file
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/lonely_def_03.rb:3: unexpected end-of-input; expected a method name https://srb.help/2001
     3 |# error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_03.rb:2: expected a delimiter to close the parameters https://srb.help/2001
     2 |def # error: Hint: this "def" token is not closed before the end of the file
           ^

test/testdata/parser/error_recovery/lonely_def_03.rb:2: expected an `end` to close the `def` statement https://srb.help/2001
     2 |def # error: Hint: this "def" token is not closed before the end of the file
           ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism provides more detailed error messages but reports 3 errors instead of 2, with some pointing to less helpful locations (column after def) rather than the def token itself.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def <method-def-name-missing><<todo method>>(&<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def 
<<todo method>>(&<blk>)
    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,6 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  def <method-def-name-missing><<todo method>>(&<blk>)
-    <emptyTree>
+  def 
+<<todo method>>(&<blk>)
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_04.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
def # error: Hint: this "def" token might not be followed by a method name
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_04.rb:3: unexpected token "end of file" https://srb.help/2001
     3 |end # error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_04.rb:2: Hint: this "def" token might not be followed by a method name https://srb.help/2003
     2 |def # error: Hint: this "def" token might not be followed by a method name
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_04.rb:4: expected an `end` to close the `def` statement https://srb.help/2001
     4 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the core issue that def has no method name, only reporting a missing end instead of identifying the malformed def statement like Original does.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def <method-def-name-missing><<todo method>>(&<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def end<<todo method>>(&<blk>)
    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  def <method-def-name-missing><<todo method>>(&<blk>)
-    <emptyTree>
+  def end<<todo method>>(&<blk>)
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_05.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def # error: Hint: this "def" token might not be followed by a method name
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_05.rb:6: unexpected token "end of file" https://srb.help/2001
     6 |end # error: unexpected token "end of file"
     7 |

test/testdata/parser/error_recovery/lonely_def_05.rb:4: Hint: this "def" token might not be followed by a method name https://srb.help/2003
     4 |  def # error: Hint: this "def" token might not be followed by a method name
          ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_05.rb:7: expected an `end` to close the `class` statement https://srb.help/2001
     7 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the core issue of the malformed def statement and only reports a generic missing end error, while Original correctly identifies the lonely def token as the root problem.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def <method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def end<<todo method>>(&<blk>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,7 +1,9 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
-    def <method-def-name-missing><<todo method>>(&<blk>)
+    def end<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_06.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A
  sig {void}
  def self # error: Hint: this "def" token might not be properly closed

  sig {void}
  def example; end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_06.rb:8: unexpected token "end of file" https://srb.help/2001
     8 |end # error: unexpected token "end of file"
     9 |

test/testdata/parser/error_recovery/lonely_def_06.rb:4: Hint: this "def" token might not be properly closed https://srb.help/2003
     4 |  def self # error: Hint: this "def" token might not be properly closed
          ^^^
    test/testdata/parser/error_recovery/lonely_def_06.rb:8: Matching `end` found here but is not indented as far
     8 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_06.rb:9: expected an `end` to close the `class` statement https://srb.help/2001
     9 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual syntax error on line 4 where def self is incomplete, only reporting a generic missing end for the class instead of identifying the malformed method definition.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self<<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def example<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self<<todo method>>(&<blk>)
      begin
        <self>.sig() do ||
          <self>.void()
        end
        def example<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -5,15 +5,16 @@
     end
 
     def self<<todo method>>(&<blk>)
-      <emptyTree>
+      begin
+        <self>.sig() do ||
+          <self>.void()
+        end
+        def example<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def example<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_07.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A # error: class definition in method body
  sig {void}
  def self.
# ^^^ error: Hint: this "def" token might not be properly closed
  #       ^ error: Hint: this "." token might not be followed by a method name

  sig {void} # error: unexpected token tLCURLY
  def example; end
end

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_07.rb:8: unexpected token tLCURLY https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
              ^

test/testdata/parser/error_recovery/lonely_def_07.rb:2: class definition in method body https://srb.help/2001
     2 |class A # error: class definition in method body
        ^^^^^

test/testdata/parser/error_recovery/lonely_def_07.rb:4: Hint: this "." token might not be followed by a method name https://srb.help/2003
     4 |  def self.
                  ^

test/testdata/parser/error_recovery/lonely_def_07.rb:4: Hint: this "def" token might not be properly closed https://srb.help/2003
     4 |  def self.
          ^^^
    test/testdata/parser/error_recovery/lonely_def_07.rb:10: Matching `end` found here but is not indented as far
    10 |end
        ^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_07.rb:8: expected a delimiter to close the parameters https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
             ^

test/testdata/parser/error_recovery/lonely_def_07.rb:8: expected a `=>` between the hash key and value https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
                   ^

test/testdata/parser/error_recovery/lonely_def_07.rb:8: unexpected '}'; expected a value in the hash literal https://srb.help/2001
     8 |  sig {void} # error: unexpected token tLCURLY
                   ^

test/testdata/parser/error_recovery/lonely_def_07.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism completely misses the actual lonely def error and instead produces confusing hash-related errors on the wrong line, failing to identify the core problem that Original correctly diagnoses.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.<method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def example<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def self.sig<<todo method>>(&<blk>)
      begin
        {<self>.void() => <emptyTree>::<C <ErrorNode>>}
        def example<<todo method>>(&<blk>)
          <emptyTree>
        end
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,16 +4,15 @@
       <self>.void()
     end
 
-    def self.<method-def-name-missing><<todo method>>(&<blk>)
-      <emptyTree>
+    def self.sig<<todo method>>(&<blk>)
+      begin
+        {<self>.void() => <emptyTree>::<C <ErrorNode>>}
+        def example<<todo method>>(&<blk>)
+          <emptyTree>
+        end
+      end
     end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
-
-    def example<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_08.rb

<details>
<summary>Input</summary>

```ruby
# typed: true
class A
  extend T::Sig
  sig {params(x: Integer).void}
  #           ^ error: Unknown parameter name `x`
  def self # error: Hint: this "def" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/lonely_def_08.rb:7: unexpected token "end of file" https://srb.help/2001
     7 |end # error: unexpected token "end of file"
     8 |

test/testdata/parser/error_recovery/lonely_def_08.rb:6: Hint: this "def" token might not be properly closed https://srb.help/2003
     6 |  def self # error: Hint: this "def" token might not be properly closed
          ^^^
    test/testdata/parser/error_recovery/lonely_def_08.rb:7: Matching `end` found here but is not indented as far
     7 |end # error: unexpected token "end of file"
        ^^^

test/testdata/parser/error_recovery/lonely_def_08.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_08.rb:6: Parameter not in method definition here
     6 |  def self # error: Hint: this "def" token might not be properly closed
          ^^^^^^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_08.rb:8: expected an `end` to close the `class` statement https://srb.help/2001
     8 |
        ^

test/testdata/parser/error_recovery/lonely_def_08.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_08.rb:6: Parameter not in method definition here
     6 |  def self # error: Hint: this "def" token might not be properly closed
          ^^^^^^^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same semantic error but misses the helpful hint about the unclosed def token, instead only reporting the missing class end at EOF which is less actionable for the actual syntax error.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def self<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def self<<todo method>>(&<blk>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -9,5 +9,7 @@
     def self<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_09.rb

<details>
<summary>Input</summary>

```ruby
# typed: true
class A
  extend T::Sig
  sig {params(x: Integer).void}
  #           ^ error: Unknown parameter name `x`
  def self.
# ^^^ error: Hint: this "def" token might not be properly closed
  #       ^ error: Hint: this "." token might not be followed by a method name
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_09.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/lonely_def_09.rb:6: Hint: this "." token might not be followed by a method name https://srb.help/2003
     6 |  def self.
                  ^

test/testdata/parser/error_recovery/lonely_def_09.rb:6: Hint: this "def" token might not be properly closed https://srb.help/2003
     6 |  def self.
          ^^^
    test/testdata/parser/error_recovery/lonely_def_09.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^

test/testdata/parser/error_recovery/lonely_def_09.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_09.rb:6: Parameter not in method definition here
     6 |  def self.
          ^^^^^^^^^
Errors: 4
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/lonely_def_09.rb:10: expected an `end` to close the `def` statement https://srb.help/2001
    10 |
        ^

test/testdata/parser/error_recovery/lonely_def_09.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^

test/testdata/parser/error_recovery/lonely_def_09.rb:4: Unknown parameter name `x` https://srb.help/5003
     4 |  sig {params(x: Integer).void}
                      ^
    test/testdata/parser/error_recovery/lonely_def_09.rb:6: Parameter not in method definition here
     6 |  def self.
     7 |# ^^^ error: Hint: this "def" token might not be properly closed
     8 |  #       ^ error: Hint: this "." token might not be followed by a method name
     9 |end # error: unexpected token "end of file"
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the same semantic error but points to EOF for syntax errors instead of the problematic def/dot tokens, making the errors less actionable than Original's specific hints about the unclosed def and missing method name.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def self.<method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <self>.extend(<emptyTree>::<C T>::<C Sig>)

    <self>.sig() do ||
      <self>.params(:x, <emptyTree>::<C Integer>).void()
    end

    def self.end<<todo method>>(&<blk>)
      <emptyTree>::<C <ErrorNode>>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -6,8 +6,8 @@
       <self>.params(:x, <emptyTree>::<C Integer>).void()
     end
 
-    def self.<method-def-name-missing><<todo method>>(&<blk>)
-      <emptyTree>
+    def self.end<<todo method>>(&<blk>)
+      <emptyTree>::<C <ErrorNode>>
     end
   end
 end
```

</details>

## lonely_def_10.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
def self # error: Hint: this "def" token is not closed before the end of the file
# error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_10.rb:3: unexpected token "end of file" https://srb.help/2001
     3 |# error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_10.rb:2: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     2 |def self # error: Hint: this "def" token is not closed before the end of the file
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_10.rb:3: expected an `end` to close the `def` statement https://srb.help/2001
     3 |# error: unexpected token "end of file"
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism provides a clearer, more concise error message that accurately identifies the missing end for the def statement, which is more helpful than Original's two separate errors.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self<<todo method>>(&<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self<<todo method>>(&<blk>)
    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def self<<todo method>>(&<blk>)
-    <emptyTree>
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_11.rb

<details>
<summary>Input</summary>

```ruby
# typed: true
def self. # error: Hint: this "def" token is not closed before the end of the file
# error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_11.rb:3: unexpected token "end of file" https://srb.help/2001
     3 |# error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_11.rb:2: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     2 |def self. # error: Hint: this "def" token is not closed before the end of the file
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (3)</summary>

```
test/testdata/parser/error_recovery/lonely_def_11.rb:3: unexpected end-of-input; expected a method name https://srb.help/2001
     3 |# error: unexpected token "end of file"
     4 |

test/testdata/parser/error_recovery/lonely_def_11.rb:2: expected a delimiter to close the parameters https://srb.help/2001
     2 |def self. # error: Hint: this "def" token is not closed before the end of the file
                 ^

test/testdata/parser/error_recovery/lonely_def_11.rb:2: expected an `end` to close the `def` statement https://srb.help/2001
     2 |def self. # error: Hint: this "def" token is not closed before the end of the file
                 ^
Errors: 3
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism provides more detailed errors but points to EOF for the first error instead of the actual problem location on line 2, and introduces an unnecessary parameters delimiter error.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self.<method-def-name-missing><<todo method>>(&<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self.
<<todo method>>(&<blk>)
    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,6 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  def self.<method-def-name-missing><<todo method>>(&<blk>)
-    <emptyTree>
+  def self.
+<<todo method>>(&<blk>)
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_12.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
def self.
  #     ^ error: Hint: this "." token might not be followed by a method name
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_12.rb:4: unexpected token "end of file" https://srb.help/2001
     4 |end # error: unexpected token "end of file"
     5 |

test/testdata/parser/error_recovery/lonely_def_12.rb:2: Hint: this "." token might not be followed by a method name https://srb.help/2003
     2 |def self.
                ^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_12.rb:5: expected an `end` to close the `def` statement https://srb.help/2001
     5 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the key error about the lonely dot operator and only reports a generic missing end, while Original correctly identifies both the problematic dot token and the unexpected EOF.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self.<method-def-name-missing><<todo method>>(&<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self.end<<todo method>>(&<blk>)
    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  def self.<method-def-name-missing><<todo method>>(&<blk>)
-    <emptyTree>
+  def self.end<<todo method>>(&<blk>)
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_13.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def self.# error: Hint: this "." token might not be followed by a method name
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/lonely_def_13.rb:6: unexpected token "end of file" https://srb.help/2001
     6 |end # error: unexpected token "end of file"
     7 |

test/testdata/parser/error_recovery/lonely_def_13.rb:4: Hint: this "." token might not be followed by a method name https://srb.help/2003
     4 |  def self.# error: Hint: this "." token might not be followed by a method name
                  ^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_13.rb:7: expected an `end` to close the `class` statement https://srb.help/2001
     7 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the primary syntax error (incomplete method definition with lonely dot operator) and only reports a generic missing end error, while Original correctly identifies both the problematic dot token and the resulting parse failure.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.<method-def-name-missing><<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def self.end<<todo method>>(&<blk>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,7 +1,9 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
-    def self.<method-def-name-missing><<todo method>>(&<blk>)
+    def self.end<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## lonely_def_14.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

# This is an example of where our greedy heuristic can sometimes do the wrong
# thing. If `def bar` had been properly closed, the `def foo` would have parsed
# ok, despite being on a new line.
#
# When we enter recovery mode, we greedily assume that seeing `def` and `foo`
# on different lines must be the cause of the syntax error, which is why the
# snapshot test shows that Sorbet recovers from this parse error by treating
# `foo` as a call in a method with a missing name, instead of as the method
# name itself.
#
# It's tests like this why all the `Hint` errors say "might," because they can
# sometimes be wrong.

class A
  def # error: Hint: this "def" token might not be followed by a method name
    foo # error: Method `foo` does not exist on `A`
  end
  def bar # error: Hint: this "def" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (4)</summary>

```
test/testdata/parser/error_recovery/lonely_def_14.rb:21: unexpected token "end of file" https://srb.help/2001
    21 |end # error: unexpected token "end of file"
    22 |

test/testdata/parser/error_recovery/lonely_def_14.rb:17: Hint: this "def" token might not be followed by a method name https://srb.help/2003
    17 |  def # error: Hint: this "def" token might not be followed by a method name
          ^^^

test/testdata/parser/error_recovery/lonely_def_14.rb:20: Hint: this "def" token might not be properly closed https://srb.help/2003
    20 |  def bar # error: Hint: this "def" token might not be properly closed
          ^^^
    test/testdata/parser/error_recovery/lonely_def_14.rb:21: Matching `end` found here but is not indented as far
    21 |end # error: unexpected token "end of file"
        ^^^

test/testdata/parser/error_recovery/lonely_def_14.rb:18: Method `foo` does not exist on `A` https://srb.help/7003
    18 |    foo # error: Method `foo` does not exist on `A`
            ^^^
  Got `A` originating from:
    test/testdata/parser/error_recovery/lonely_def_14.rb:17:
    17 |  def # error: Hint: this "def" token might not be followed by a method name
          ^
Errors: 4
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/lonely_def_14.rb:22: expected an `end` to close the `class` statement https://srb.help/2001
    22 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism only reports a generic missing end error at EOF, completely missing the actual syntax issues with the malformed def statements that Original correctly identifies with helpful hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def <method-def-name-missing><<todo method>>(&<blk>)
      <self>.foo()
    end

    def bar<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def foo<<todo method>>(&<blk>)
      <emptyTree>
    end

    def bar<<todo method>>(&<blk>)
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,13 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
-    def <method-def-name-missing><<todo method>>(&<blk>)
-      <self>.foo()
+    def foo<<todo method>>(&<blk>)
+      <emptyTree>
     end
 
     def bar<<todo method>>(&<blk>)
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## long_line_indent.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
def foo
  really_long_method_________________________________________________________________________________________________________ do
  #                                                                                                                           ^^ error: Hint: this "do" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/long_line_indent.rb:5: unexpected token "end of file" https://srb.help/2001
     5 |end # error: unexpected token "end of file"
     6 |

test/testdata/parser/error_recovery/long_line_indent.rb:3: Hint: this "do" token might not be properly closed https://srb.help/2003
     3 |  really_long_method_________________________________________________________________________________________________________ do
                                                                                                                                      ^^
    test/testdata/parser/error_recovery/long_line_indent.rb:5: Matching `end` found here but is not indented as far
     5 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/long_line_indent.rb:6: expected an `end` to close the `def` statement https://srb.help/2001
     6 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual problem (unclosed do block) and only reports the missing end for def, while Original correctly identifies both the unclosed do block and provides helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    <self>.really_long_method_________________________________________________________________________________________________________() do ||
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    begin
      <self>.really_long_method_________________________________________________________________________________________________________() do ||
        <emptyTree>
      end
      <emptyTree>::<C <ErrorNode>>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,7 +1,10 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def foo<<todo method>>(&<blk>)
-    <self>.really_long_method_________________________________________________________________________________________________________() do ||
-      <emptyTree>
+    begin
+      <self>.really_long_method_________________________________________________________________________________________________________() do ||
+        <emptyTree>
+      end
+      <emptyTree>::<C <ErrorNode>>
     end
   end
 end
```

</details>

## long_prefix_indent.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
# next line is indented 101 spaces
                                                                                                     def foo # error: Hint: this "def" token is not closed before the end of the file
                                                                                                       1.times do
                                                                                                     end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/long_prefix_indent.rb:5: unexpected token "end of file" https://srb.help/2001
     5 |                                                                                                     end # error: unexpected token "end of file"
     6 |

test/testdata/parser/error_recovery/long_prefix_indent.rb:3: Hint: this "def" token is not closed before the end of the file https://srb.help/2003
     3 |                                                                                                     def foo # error: Hint: this "def" token is not closed before the end of the file
                                                                                                             ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/long_prefix_indent.rb:6: expected an `end` to close the `def` statement https://srb.help/2001
     6 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches the unclosed def but points to EOF instead of the actual def token location, and misses the mismatched end token error that Original reports.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  1.times() do ||
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    begin
      1.times() do ||
        <emptyTree>
      end
      <emptyTree>::<C <ErrorNode>>
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,10 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  1.times() do ||
-    <emptyTree>
+  def foo<<todo method>>(&<blk>)
+    begin
+      1.times() do ||
+        <emptyTree>
+      end
+      <emptyTree>::<C <ErrorNode>>
+    end
   end
 end
```

</details>

## missing_fun.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

def test_method_without_fun_name(x)
  x.
end # error: unexpected token

def test_method_without_fun_name_plus_before(x)
  before = 1
  x.
end # error: unexpected token

def test_method_without_fun_name_plus_after(x)
  x.
  after = 1
end

def test_method_without_fun_name_before_and_after(x)
  before = 1
  x.
  after = 1
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/missing_fun.rb:5: unexpected token "end" https://srb.help/2001
     5 |end # error: unexpected token
        ^^^

test/testdata/parser/error_recovery/missing_fun.rb:10: unexpected token "end" https://srb.help/2001
    10 |end # error: unexpected token
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/missing_fun.rb:22: expected an `end` to close the `def` statement https://srb.help/2001
    22 |
        ^

test/testdata/parser/error_recovery/missing_fun.rb:22: expected an `end` to close the `def` statement https://srb.help/2001
    22 |
        ^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the actual syntax errors on lines 4 and 9 where incomplete method calls exist, instead reporting generic missing end errors at EOF, while Original correctly identifies the problematic incomplete expressions.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_method_without_fun_name<<todo method>>(x, &<blk>)
    x.<method-name-missing>()
  end

  def test_method_without_fun_name_plus_before<<todo method>>(x, &<blk>)
    begin
      before = 1
      x.<method-name-missing>()
    end
  end

  def test_method_without_fun_name_plus_after<<todo method>>(x, &<blk>)
    x.after=(1)
  end

  def test_method_without_fun_name_before_and_after<<todo method>>(x, &<blk>)
    begin
      before = 1
      x.after=(1)
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_method_without_fun_name<<todo method>>(x, &<blk>)
    begin
      x.end()
      def test_method_without_fun_name_plus_before<<todo method>>(x, &<blk>)
        begin
          before = 1
          x.end()
          def test_method_without_fun_name_plus_after<<todo method>>(x, &<blk>)
            x.after=(1)
          end
          def test_method_without_fun_name_before_and_after<<todo method>>(x, &<blk>)
            begin
              before = 1
              x.after=(1)
            end
          end
          <emptyTree>::<C <ErrorNode>>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,23 +1,23 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   def test_method_without_fun_name<<todo method>>(x, &<blk>)
-    x.<method-name-missing>()
-  end
-
-  def test_method_without_fun_name_plus_before<<todo method>>(x, &<blk>)
     begin
-      before = 1
-      x.<method-name-missing>()
+      x.end()
+      def test_method_without_fun_name_plus_before<<todo method>>(x, &<blk>)
+        begin
+          before = 1
+          x.end()
+          def test_method_without_fun_name_plus_after<<todo method>>(x, &<blk>)
+            x.after=(1)
+          end
+          def test_method_without_fun_name_before_and_after<<todo method>>(x, &<blk>)
+            begin
+              before = 1
+              x.after=(1)
+            end
+          end
+          <emptyTree>::<C <ErrorNode>>
+        end
+      end
     end
   end
-
-  def test_method_without_fun_name_plus_after<<todo method>>(x, &<blk>)
-    x.after=(1)
-  end
-
-  def test_method_without_fun_name_before_and_after<<todo method>>(x, &<blk>)
-    begin
-      before = 1
-      x.after=(1)
-    end
-  end
 end
```

</details>

## missing_operator.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module Opus::Log
  def self.info; end
end

class A
  def g
    t = T.let(true, T::Boolean)
    f = T.let(false, T::Boolean)
    puts(0 .. 0)
    puts(0 .. )
    if 0 .. # error: unexpected token "if"
    end
    if 0 .. # error: Unsupported node type `IFlipflop`
      #  ^^ error: missing arg to ".." operator
      puts 'hello'
    end
    puts(1 ... 1)
    puts(1 ... )
    if 1... # error: unexpected token "if"
    end
    if 1... # error: Unsupported node type `EFlipflop`
      # ^^^ error: missing arg to "..." operator
      puts 'hello'
    end
    puts(2 + 2)
    puts(2 + ) # error: missing arg to "+" operator
    if 2 + # error: missing arg to "+" operator
    end
    if 2 + # error: missing arg to "+" operator
      puts 'hello'
    end
    puts(3 - 3)
    puts(3 - ) # error: missing arg to "-" operator
    if 3 - # error: missing arg to "-" operator
    end
    if 3 - # error: missing arg to "-" operator
      puts 'hello'
    end
    puts(4 * 4)
    puts(4 * ) # error: missing arg to "*" operator
    if 4 * # error: missing arg to "*" operator
    end
    if 4 * # error: missing arg to "*" operator
      puts 'hello'
    end
    puts(5 / 5)
    puts(5 / ) # error: missing arg to "/" operator
    if 5 / # error: missing arg to "/" operator
    end
    if 5 / # error: missing arg to "/" operator
      puts 'hello'
    end
    puts(6 % 6)
    puts(6 % ) # error: missing arg to "%" operator
    if 6 % # error: missing arg to "%" operator
    end
    if 6 % # error: missing arg to "%" operator
      puts 'hello'
    end
    puts(7 ** 7)
    puts(7 ** ) # error: missing arg to "**" operator
    if 7 ** # error: missing arg to "**" operator
    end
    if 7 ** # error: missing arg to "**" operator
      puts 'hello'
    end
    puts(-8 ** 8)
    puts(-8 ** ) # error: missing arg to "**" operator
    if -8 ** # error: missing arg to "**" operator
    end
    if -8 ** # error: missing arg to "**" operator
      puts 'hello'
    end
    puts(+9 ** 9)
    puts(+9 ** ) # error: missing arg to "**" operator
    if +9 ** # error: missing arg to "**" operator
    end
    if +9 ** # error: missing arg to "**" operator
      puts 'hello'
    end
    puts(-10)
    puts(-) # error: missing arg to "unary -" operator
    if - # error: missing arg to "unary -" operator
    end
    if - # error: missing arg to "unary -" operator
      puts 'hello'
    end
    puts(+10)
    puts(+) # error: missing arg to "unary +" operator
    if + # error: missing arg to "unary +" operator
    end
    if + # error: missing arg to "unary +" operator
      puts 'hello'
    end
    puts(10 | 10)
    puts(10 | ) # error: missing arg to "|" operator
    if 10 | # error: missing arg to "|" operator
    end
    if 10 | # error: missing arg to "|" operator
      puts 'hello'
    end
    puts(11 ^ 11)
    puts(11 ^ ) # error: missing arg to "^" operator
    if 11 ^ # error: missing arg to "^" operator
    end
    if 11 ^ # error: missing arg to "^" operator
      puts 'hello'
    end
    puts(12 & 12)
    puts(12 & ) # error: missing arg to "&" operator
    if 12 & # error: missing arg to "&" operator
    end
    if 12 & # error: missing arg to "&" operator
      puts 'hello'
    end
    puts(13 <=> 13)
    puts(13 <=> ) # error: missing arg to "<=>" operator
    if 13 <=> # error: missing arg to "<=>" operator
    end
    if 13 <=> # error: missing arg to "<=>" operator
      puts 'hello'
    end
    puts(14 == 14)
    puts(14 == ) # error: missing arg to "==" operator
    if 14 == # error: missing arg to "==" operator
    end
    if 14 == # error: missing arg to "==" operator
      puts 'hello'
    end
    puts(15 === 15)
    puts(15 === ) # error: missing arg to "===" operator
    if 15 === # error: missing arg to "===" operator
    end
    if 15 === # error: missing arg to "===" operator
      puts 'hello'
    end
    puts(16 != 16)
    puts(16 != ) # error: missing arg to "!=" operator
    if 16 != # error: missing arg to "!=" operator
    end
    if 16 != # error: missing arg to "!=" operator
      puts 'hello'
    end
    puts(/17/ =~ "17")
    puts(/17/ =~ ) # error: missing arg to "=~" operator
    if /17/ =~ # error: missing arg to "=~" operator
    end
    if /17/ =~ # error: missing arg to "=~" operator
      puts 'hello'
    end
    puts(/18/ !~ "eighteen")
    puts(/18/ !~ ) # error: missing arg to "!~" operator
    if /18/ !~ # error: missing arg to "!~" operator
    end
    if /18/ !~ # error: missing arg to "!~" operator
      puts 'hello'
    end
    puts(!t)
    puts(!) # error: missing arg to "!" operator
    if ! # error: missing arg to "!" operator
    end
    if !
      puts 'hello'
    end
    puts(~19)
    puts(~) # error: missing arg to "~" operator
    if ~ # error: missing arg to "~" operator
    end
    if ~ # error: missing arg to "~" operator
      puts 'hello'
    end
    puts(20 << 20)
    puts(20 << ) # error: missing arg to "<<" operator
    if 20 << # error: missing arg to "<<" operator
    end
    if 20 << # error: missing arg to "<<" operator
      puts 'hello'
    end
    puts(21 >> 21)
    puts(21 >> ) # error: missing arg to ">>" operator
    if 21 >> # error: missing arg to ">>" operator
    end
    if 21 >> # error: missing arg to ">>" operator
      puts 'hello'
    end
    puts(t && t)
    puts(t && ) # error: missing arg to "&&" operator
    if t && # error: missing arg to "&&" operator
    end
    if t && # error: missing arg to "&&" operator
      puts 'hello'
    end
    puts(f || f)
    puts(f || ) # error: missing arg to "||" operator
    if f || # error: missing arg to "||" operator
    end
    if f || # error: missing arg to "||" operator
      puts 'hello'
    end
    puts(24 > 24)
    puts(24 > ) # error: missing arg to ">" operator
    if 24 > # error: missing arg to ">" operator
    end
    if 24 > # error: missing arg to ">" operator
      puts 'hello'
    end
    puts(25 < 25)
    puts(25 < ) # error: missing arg to "<" operator
    if 25 < # error: missing arg to "<" operator
    end
    if 25 < # error: missing arg to "<" operator
      puts 'hello'
    end
    puts(26 >= 26)
    puts(26 >= ) # error: missing arg to ">=" operator
    if 26 >= # error: missing arg to ">=" operator
    end
    if 26 >= # error: missing arg to ">=" operator
      puts 'hello'
    end
    puts(27 <= 27)
    puts(27 <= ) # error: missing arg to "<=" operator
    if 27 <= # error: missing arg to "<=" operator
    end
    if 27 <= # error: missing arg to "<=" operator
      puts 'hello'
    end
  end # error: unexpected token "end"
end

```

</details>

<details>
<summary>Original errors (93)</summary>

```
test/testdata/parser/error_recovery/missing_operator.rb:13: unexpected token "if" https://srb.help/2001
    13 |    if 0 .. # error: unexpected token "if"
            ^^

test/testdata/parser/error_recovery/missing_operator.rb:15: missing arg to ".." operator https://srb.help/2001
    15 |    if 0 .. # error: Unsupported node type `IFlipflop`
                 ^^

test/testdata/parser/error_recovery/missing_operator.rb:21: unexpected token "if" https://srb.help/2001
    21 |    if 1... # error: unexpected token "if"
            ^^

test/testdata/parser/error_recovery/missing_operator.rb:23: missing arg to "..." operator https://srb.help/2001
    23 |    if 1... # error: Unsupported node type `EFlipflop`
                ^^^

test/testdata/parser/error_recovery/missing_operator.rb:28: missing arg to "+" operator https://srb.help/2001
    28 |    puts(2 + ) # error: missing arg to "+" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:29: missing arg to "+" operator https://srb.help/2001
    29 |    if 2 + # error: missing arg to "+" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:31: missing arg to "+" operator https://srb.help/2001
    31 |    if 2 + # error: missing arg to "+" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:35: missing arg to "-" operator https://srb.help/2001
    35 |    puts(3 - ) # error: missing arg to "-" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:36: missing arg to "-" operator https://srb.help/2001
    36 |    if 3 - # error: missing arg to "-" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:38: missing arg to "-" operator https://srb.help/2001
    38 |    if 3 - # error: missing arg to "-" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:42: missing arg to "*" operator https://srb.help/2001
    42 |    puts(4 * ) # error: missing arg to "*" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:43: missing arg to "*" operator https://srb.help/2001
    43 |    if 4 * # error: missing arg to "*" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:45: missing arg to "*" operator https://srb.help/2001
    45 |    if 4 * # error: missing arg to "*" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:49: missing arg to "/" operator https://srb.help/2001
    49 |    puts(5 / ) # error: missing arg to "/" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:50: missing arg to "/" operator https://srb.help/2001
    50 |    if 5 / # error: missing arg to "/" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:52: missing arg to "/" operator https://srb.help/2001
    52 |    if 5 / # error: missing arg to "/" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:56: missing arg to "%" operator https://srb.help/2001
    56 |    puts(6 % ) # error: missing arg to "%" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:57: missing arg to "%" operator https://srb.help/2001
    57 |    if 6 % # error: missing arg to "%" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:59: missing arg to "%" operator https://srb.help/2001
    59 |    if 6 % # error: missing arg to "%" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:63: missing arg to "**" operator https://srb.help/2001
    63 |    puts(7 ** ) # error: missing arg to "**" operator
                   ^^

test/testdata/parser/error_recovery/missing_operator.rb:64: missing arg to "**" operator https://srb.help/2001
    64 |    if 7 ** # error: missing arg to "**" operator
                 ^^

test/testdata/parser/error_recovery/missing_operator.rb:66: missing arg to "**" operator https://srb.help/2001
    66 |    if 7 ** # error: missing arg to "**" operator
                 ^^

test/testdata/parser/error_recovery/missing_operator.rb:70: missing arg to "**" operator https://srb.help/2001
    70 |    puts(-8 ** ) # error: missing arg to "**" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:71: missing arg to "**" operator https://srb.help/2001
    71 |    if -8 ** # error: missing arg to "**" operator
                  ^^

test/testdata/parser/error_recovery/missing_operator.rb:73: missing arg to "**" operator https://srb.help/2001
    73 |    if -8 ** # error: missing arg to "**" operator
                  ^^

test/testdata/parser/error_recovery/missing_operator.rb:77: missing arg to "**" operator https://srb.help/2001
    77 |    puts(+9 ** ) # error: missing arg to "**" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:78: missing arg to "**" operator https://srb.help/2001
    78 |    if +9 ** # error: missing arg to "**" operator
                  ^^

test/testdata/parser/error_recovery/missing_operator.rb:80: missing arg to "**" operator https://srb.help/2001
    80 |    if +9 ** # error: missing arg to "**" operator
                  ^^

test/testdata/parser/error_recovery/missing_operator.rb:84: missing arg to "unary -" operator https://srb.help/2001
    84 |    puts(-) # error: missing arg to "unary -" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:85: missing arg to "unary -" operator https://srb.help/2001
    85 |    if - # error: missing arg to "unary -" operator
               ^

test/testdata/parser/error_recovery/missing_operator.rb:87: missing arg to "unary -" operator https://srb.help/2001
    87 |    if - # error: missing arg to "unary -" operator
               ^

test/testdata/parser/error_recovery/missing_operator.rb:91: missing arg to "unary +" operator https://srb.help/2001
    91 |    puts(+) # error: missing arg to "unary +" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:92: missing arg to "unary +" operator https://srb.help/2001
    92 |    if + # error: missing arg to "unary +" operator
               ^

test/testdata/parser/error_recovery/missing_operator.rb:94: missing arg to "unary +" operator https://srb.help/2001
    94 |    if + # error: missing arg to "unary +" operator
               ^

test/testdata/parser/error_recovery/missing_operator.rb:98: missing arg to "|" operator https://srb.help/2001
    98 |    puts(10 | ) # error: missing arg to "|" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:99: missing arg to "|" operator https://srb.help/2001
    99 |    if 10 | # error: missing arg to "|" operator
                  ^

test/testdata/parser/error_recovery/missing_operator.rb:101: missing arg to "|" operator https://srb.help/2001
     101 |    if 10 | # error: missing arg to "|" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:105: missing arg to "^" operator https://srb.help/2001
     105 |    puts(11 ^ ) # error: missing arg to "^" operator
                      ^

test/testdata/parser/error_recovery/missing_operator.rb:106: missing arg to "^" operator https://srb.help/2001
     106 |    if 11 ^ # error: missing arg to "^" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:108: missing arg to "^" operator https://srb.help/2001
     108 |    if 11 ^ # error: missing arg to "^" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:112: missing arg to "&" operator https://srb.help/2001
     112 |    puts(12 & ) # error: missing arg to "&" operator
                      ^

test/testdata/parser/error_recovery/missing_operator.rb:113: missing arg to "&" operator https://srb.help/2001
     113 |    if 12 & # error: missing arg to "&" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:115: missing arg to "&" operator https://srb.help/2001
     115 |    if 12 & # error: missing arg to "&" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:119: missing arg to "<=>" operator https://srb.help/2001
     119 |    puts(13 <=> ) # error: missing arg to "<=>" operator
                      ^^^

test/testdata/parser/error_recovery/missing_operator.rb:120: missing arg to "<=>" operator https://srb.help/2001
     120 |    if 13 <=> # error: missing arg to "<=>" operator
                    ^^^

test/testdata/parser/error_recovery/missing_operator.rb:122: missing arg to "<=>" operator https://srb.help/2001
     122 |    if 13 <=> # error: missing arg to "<=>" operator
                    ^^^

test/testdata/parser/error_recovery/missing_operator.rb:126: missing arg to "==" operator https://srb.help/2001
     126 |    puts(14 == ) # error: missing arg to "==" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:127: missing arg to "==" operator https://srb.help/2001
     127 |    if 14 == # error: missing arg to "==" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:129: missing arg to "==" operator https://srb.help/2001
     129 |    if 14 == # error: missing arg to "==" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:133: missing arg to "===" operator https://srb.help/2001
     133 |    puts(15 === ) # error: missing arg to "===" operator
                      ^^^

test/testdata/parser/error_recovery/missing_operator.rb:134: missing arg to "===" operator https://srb.help/2001
     134 |    if 15 === # error: missing arg to "===" operator
                    ^^^

test/testdata/parser/error_recovery/missing_operator.rb:136: missing arg to "===" operator https://srb.help/2001
     136 |    if 15 === # error: missing arg to "===" operator
                    ^^^

test/testdata/parser/error_recovery/missing_operator.rb:140: missing arg to "!=" operator https://srb.help/2001
     140 |    puts(16 != ) # error: missing arg to "!=" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:141: missing arg to "!=" operator https://srb.help/2001
     141 |    if 16 != # error: missing arg to "!=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:143: missing arg to "!=" operator https://srb.help/2001
     143 |    if 16 != # error: missing arg to "!=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:147: missing arg to "=~" operator https://srb.help/2001
     147 |    puts(/17/ =~ ) # error: missing arg to "=~" operator
                        ^^

test/testdata/parser/error_recovery/missing_operator.rb:148: missing arg to "=~" operator https://srb.help/2001
     148 |    if /17/ =~ # error: missing arg to "=~" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:150: missing arg to "=~" operator https://srb.help/2001
     150 |    if /17/ =~ # error: missing arg to "=~" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:154: missing arg to "!~" operator https://srb.help/2001
     154 |    puts(/18/ !~ ) # error: missing arg to "!~" operator
                        ^^

test/testdata/parser/error_recovery/missing_operator.rb:155: missing arg to "!~" operator https://srb.help/2001
     155 |    if /18/ !~ # error: missing arg to "!~" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:157: missing arg to "!~" operator https://srb.help/2001
     157 |    if /18/ !~ # error: missing arg to "!~" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:161: missing arg to "!" operator https://srb.help/2001
     161 |    puts(!) # error: missing arg to "!" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:162: missing arg to "!" operator https://srb.help/2001
     162 |    if ! # error: missing arg to "!" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:168: missing arg to "~" operator https://srb.help/2001
     168 |    puts(~) # error: missing arg to "~" operator
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:169: missing arg to "~" operator https://srb.help/2001
     169 |    if ~ # error: missing arg to "~" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:171: missing arg to "~" operator https://srb.help/2001
     171 |    if ~ # error: missing arg to "~" operator
                 ^

test/testdata/parser/error_recovery/missing_operator.rb:175: missing arg to "<<" operator https://srb.help/2001
     175 |    puts(20 << ) # error: missing arg to "<<" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:176: missing arg to "<<" operator https://srb.help/2001
     176 |    if 20 << # error: missing arg to "<<" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:178: missing arg to "<<" operator https://srb.help/2001
     178 |    if 20 << # error: missing arg to "<<" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:182: missing arg to ">>" operator https://srb.help/2001
     182 |    puts(21 >> ) # error: missing arg to ">>" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:183: missing arg to ">>" operator https://srb.help/2001
     183 |    if 21 >> # error: missing arg to ">>" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:185: missing arg to ">>" operator https://srb.help/2001
     185 |    if 21 >> # error: missing arg to ">>" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:189: missing arg to "&&" operator https://srb.help/2001
     189 |    puts(t && ) # error: missing arg to "&&" operator
                     ^^

test/testdata/parser/error_recovery/missing_operator.rb:190: missing arg to "&&" operator https://srb.help/2001
     190 |    if t && # error: missing arg to "&&" operator
                   ^^

test/testdata/parser/error_recovery/missing_operator.rb:192: missing arg to "&&" operator https://srb.help/2001
     192 |    if t && # error: missing arg to "&&" operator
                   ^^

test/testdata/parser/error_recovery/missing_operator.rb:196: missing arg to "||" operator https://srb.help/2001
     196 |    puts(f || ) # error: missing arg to "||" operator
                     ^^

test/testdata/parser/error_recovery/missing_operator.rb:197: missing arg to "||" operator https://srb.help/2001
     197 |    if f || # error: missing arg to "||" operator
                   ^^

test/testdata/parser/error_recovery/missing_operator.rb:199: missing arg to "||" operator https://srb.help/2001
     199 |    if f || # error: missing arg to "||" operator
                   ^^

test/testdata/parser/error_recovery/missing_operator.rb:203: missing arg to ">" operator https://srb.help/2001
     203 |    puts(24 > ) # error: missing arg to ">" operator
                      ^

test/testdata/parser/error_recovery/missing_operator.rb:204: missing arg to ">" operator https://srb.help/2001
     204 |    if 24 > # error: missing arg to ">" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:206: missing arg to ">" operator https://srb.help/2001
     206 |    if 24 > # error: missing arg to ">" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:210: missing arg to "<" operator https://srb.help/2001
     210 |    puts(25 < ) # error: missing arg to "<" operator
                      ^

test/testdata/parser/error_recovery/missing_operator.rb:211: missing arg to "<" operator https://srb.help/2001
     211 |    if 25 < # error: missing arg to "<" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:213: missing arg to "<" operator https://srb.help/2001
     213 |    if 25 < # error: missing arg to "<" operator
                    ^

test/testdata/parser/error_recovery/missing_operator.rb:217: missing arg to ">=" operator https://srb.help/2001
     217 |    puts(26 >= ) # error: missing arg to ">=" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:218: missing arg to ">=" operator https://srb.help/2001
     218 |    if 26 >= # error: missing arg to ">=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:220: missing arg to ">=" operator https://srb.help/2001
     220 |    if 26 >= # error: missing arg to ">=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:224: missing arg to "<=" operator https://srb.help/2001
     224 |    puts(27 <= ) # error: missing arg to "<=" operator
                      ^^

test/testdata/parser/error_recovery/missing_operator.rb:225: missing arg to "<=" operator https://srb.help/2001
     225 |    if 27 <= # error: missing arg to "<=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:227: missing arg to "<=" operator https://srb.help/2001
     227 |    if 27 <= # error: missing arg to "<=" operator
                    ^^

test/testdata/parser/error_recovery/missing_operator.rb:230: unexpected token "end" https://srb.help/2001
     230 |  end # error: unexpected token "end"
            ^^^

test/testdata/parser/error_recovery/missing_operator.rb:15: Unsupported node type `IFlipflop` https://srb.help/3003
    15 |    if 0 .. # error: Unsupported node type `IFlipflop`
    16 |      #  ^^ error: missing arg to ".." operator
    17 |      puts 'hello'

test/testdata/parser/error_recovery/missing_operator.rb:23: Unsupported node type `EFlipflop` https://srb.help/3003
    23 |    if 1... # error: Unsupported node type `EFlipflop`
    24 |      # ^^^ error: missing arg to "..." operator
    25 |      puts 'hello'
Errors: 93
```

</details>

<details>
<summary>Prism errors (13)</summary>

```
test/testdata/parser/error_recovery/missing_operator.rb:14: expected `then` or `;` or '\n' https://srb.help/2001
    14 |    end
            ^^^

test/testdata/parser/error_recovery/missing_operator.rb:17: expected `then` or `;` or '\n' https://srb.help/2001
    17 |      puts 'hello'
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:22: expected `then` or `;` or '\n' https://srb.help/2001
    22 |    end
            ^^^

test/testdata/parser/error_recovery/missing_operator.rb:25: expected `then` or `;` or '\n' https://srb.help/2001
    25 |      puts 'hello'
                   ^

test/testdata/parser/error_recovery/missing_operator.rb:28: unexpected ')'; expected an expression after the operator https://srb.help/2001
    28 |    puts(2 + ) # error: missing arg to "+" operator
                     ^

test/testdata/parser/error_recovery/missing_operator.rb:30: unexpected 'end'; expected an expression after the operator https://srb.help/2001
    30 |    end
            ^^^

test/testdata/parser/error_recovery/missing_operator.rb:30: expected `then` or `;` or '\n' https://srb.help/2001
    30 |    end
            ^^^

test/testdata/parser/error_recovery/missing_operator.rb:30: expected an `end` to close the `def` statement https://srb.help/2001
    30 |    end
               ^

test/testdata/parser/error_recovery/missing_operator.rb:30: expected an `end` to close the `class` statement https://srb.help/2001
    30 |    end
               ^

test/testdata/parser/error_recovery/missing_operator.rb:13: Unsupported node type `IFlipflop` https://srb.help/3003
    13 |    if 0 .. # error: unexpected token "if"
               ^^^^

test/testdata/parser/error_recovery/missing_operator.rb:15: Unsupported node type `IFlipflop` https://srb.help/3003
    15 |    if 0 .. # error: Unsupported node type `IFlipflop`
    16 |      #  ^^ error: missing arg to ".." operator
    17 |      puts 'hello'

test/testdata/parser/error_recovery/missing_operator.rb:21: Unsupported node type `EFlipflop` https://srb.help/3003
    21 |    if 1... # error: unexpected token "if"
               ^^^^

test/testdata/parser/error_recovery/missing_operator.rb:23: Unsupported node type `EFlipflop` https://srb.help/3003
    23 |    if 1... # error: Unsupported node type `EFlipflop`
    24 |      # ^^^ error: missing arg to "..." operator
    25 |      puts 'hello'
Errors: 13
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses 80+ operator errors that Original correctly catches, only reporting 13 errors versus Original's 93, failing to detect most missing operator arguments.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def g<<todo method>>(&<blk>)
      begin
        t = <emptyTree>::<C T>.let(true, <emptyTree>::<C T>::<C Boolean>)
        f = <emptyTree>::<C T>.let(false, <emptyTree>::<C T>::<C Boolean>)
        <self>.puts(::<Magic>.<build-range>(0, 0, false))
        <self>.puts(::<Magic>.<build-range>(0, <emptyTree>, false))
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    if <emptyTree>
      <self>.puts("hello")
    else
      <emptyTree>
    end

    <self>.puts(::<Magic>.<build-range>(1, 1, true))

    <self>.puts(::<Magic>.<build-range>(1, <emptyTree>, true))

    if <emptyTree>::<C <ErrorNode>>
      <emptyTree>
    else
      <emptyTree>
    end
  end

  if <emptyTree>
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(2.+(2))

  <self>.puts(2.+(<emptyTree>::<C <ErrorNode>>))

  if 2.+(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 2.+(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(3.-(3))

  <self>.puts(3.-(<emptyTree>::<C <ErrorNode>>))

  if 3.-(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 3.-(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(4.*(4))

  <self>.puts(4.*(<emptyTree>::<C <ErrorNode>>))

  if 4.*(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 4.*(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(5./(5))

  <self>.puts(5./(<emptyTree>::<C <ErrorNode>>))

  if 5./(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 5./(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(6.%(6))

  <self>.puts(6.%(<emptyTree>::<C <ErrorNode>>))

  if 6.%(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 6.%(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(7.**(7))

  <self>.puts(7.**(<emptyTree>::<C <ErrorNode>>))

  if 7.**(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 7.**(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(8.**(8).-@())

  <self>.puts(8.**(<emptyTree>::<C <ErrorNode>>).-@())

  if 8.**(<emptyTree>::<C <ErrorNode>>).-@()
    <emptyTree>
  else
    <emptyTree>
  end

  if 8.**(<emptyTree>::<C <ErrorNode>>).-@()
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(9.**(9).+@())

  <self>.puts(9.**(<emptyTree>::<C <ErrorNode>>).+@())

  if 9.**(<emptyTree>::<C <ErrorNode>>).+@()
    <emptyTree>
  else
    <emptyTree>
  end

  if 9.**(<emptyTree>::<C <ErrorNode>>).+@()
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(-10)

  <self>.puts(<emptyTree>::<C <ErrorNode>>.-@())

  if <emptyTree>::<C <ErrorNode>>.-@()
    <emptyTree>
  else
    <emptyTree>
  end

  if <emptyTree>::<C <ErrorNode>>.-@()
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(10)

  <self>.puts(<emptyTree>::<C <ErrorNode>>.+@())

  if <emptyTree>::<C <ErrorNode>>.+@()
    <emptyTree>
  else
    <emptyTree>
  end

  if <emptyTree>::<C <ErrorNode>>.+@()
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(10.|(10))

  <self>.puts(10.|(<emptyTree>::<C <ErrorNode>>))

  if 10.|(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 10.|(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(11.^(11))

  <self>.puts(11.^(<emptyTree>::<C <ErrorNode>>))

  if 11.^(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 11.^(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(12.&(12))

  <self>.puts(12.&(<emptyTree>::<C <ErrorNode>>))

  if 12.&(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 12.&(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(13.<=>(13))

  <self>.puts(13.<=>(<emptyTree>::<C <ErrorNode>>))

  if 13.<=>(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 13.<=>(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(14.==(14))

  <self>.puts(14.==(<emptyTree>::<C <ErrorNode>>))

  if 14.==(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 14.==(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(15.===(15))

  <self>.puts(15.===(<emptyTree>::<C <ErrorNode>>))

  if 15.===(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 15.===(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(16.!=(16))

  <self>.puts(16.!=(<emptyTree>::<C <ErrorNode>>))

  if 16.!=(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 16.!=(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(::Regexp.new("17", 0).=~("17"))

  <self>.puts(::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>))

  if ::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if ::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(::Regexp.new("18", 0).!~("eighteen"))

  <self>.puts(::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>))

  if ::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if ::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(<self>.t().!())

  <self>.puts(<emptyTree>::<C <ErrorNode>>.!())

  if <emptyTree>::<C <ErrorNode>>.!()
    <emptyTree>
  else
    <emptyTree>
  end

  if <self>.puts("hello").!()
    <emptyTree>
  else
    <emptyTree>
  end

  <self>.puts(19.~())

  <self>.puts(<emptyTree>::<C <ErrorNode>>.~())

  if <emptyTree>::<C <ErrorNode>>.~()
    <emptyTree>
  else
    <emptyTree>
  end

  if <emptyTree>::<C <ErrorNode>>.~()
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(20.<<(20))

  <self>.puts(20.<<(<emptyTree>::<C <ErrorNode>>))

  if 20.<<(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 20.<<(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(21.>>(21))

  <self>.puts(21.>>(<emptyTree>::<C <ErrorNode>>))

  if 21.>>(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 21.>>(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(begin
      &&$2 = <self>.t()
      if &&$2
        <self>.t()
      else
        &&$2
      end
    end)

  <self>.puts(begin
      &&$3 = <self>.t()
      if &&$3
        <emptyTree>::<C <ErrorNode>>
      else
        &&$3
      end
    end)

  if begin
      &&$4 = <self>.t()
      if &&$4
        <emptyTree>::<C <ErrorNode>>
      else
        &&$4
      end
    end
    <emptyTree>
  else
    <emptyTree>
  end

  if begin
      &&$5 = <self>.t()
      if &&$5
        <emptyTree>::<C <ErrorNode>>
      else
        &&$5
      end
    end
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(begin
      ||$6 = <self>.f()
      if ||$6
        ||$6
      else
        <self>.f()
      end
    end)

  <self>.puts(begin
      ||$7 = <self>.f()
      if ||$7
        ||$7
      else
        <emptyTree>::<C <ErrorNode>>
      end
    end)

  if begin
      ||$8 = <self>.f()
      if ||$8
        ||$8
      else
        <emptyTree>::<C <ErrorNode>>
      end
    end
    <emptyTree>
  else
    <emptyTree>
  end

  if begin
      ||$9 = <self>.f()
      if ||$9
        ||$9
      else
        <emptyTree>::<C <ErrorNode>>
      end
    end
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(24.>(24))

  <self>.puts(24.>(<emptyTree>::<C <ErrorNode>>))

  if 24.>(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 24.>(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(25.<(25))

  <self>.puts(25.<(<emptyTree>::<C <ErrorNode>>))

  if 25.<(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 25.<(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(26.>=(26))

  <self>.puts(26.>=(<emptyTree>::<C <ErrorNode>>))

  if 26.>=(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 26.>=(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end

  <self>.puts(27.<=(27))

  <self>.puts(27.<=(<emptyTree>::<C <ErrorNode>>))

  if 27.<=(<emptyTree>::<C <ErrorNode>>)
    <emptyTree>
  else
    <emptyTree>
  end

  if 27.<=(<emptyTree>::<C <ErrorNode>>)
    <self>.puts("hello")
  else
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def g<<todo method>>(&<blk>)
      begin
        t = <emptyTree>::<C T>.let(true, <emptyTree>::<C T>::<C Boolean>)
        f = <emptyTree>::<C T>.let(false, <emptyTree>::<C T>::<C Boolean>)
        <self>.puts(::<Magic>.<build-range>(0, 0, false))
        <self>.puts(::<Magic>.<build-range>(0, <emptyTree>, false))
        if <emptyTree>
          <emptyTree>
        else
          <emptyTree>
        end
        if <emptyTree>
          "hello"
        else
          <emptyTree>
        end
        <self>.puts(::<Magic>.<build-range>(1, 1, true))
        <self>.puts(::<Magic>.<build-range>(1, <emptyTree>, true))
        if <emptyTree>
          <emptyTree>
        else
          <emptyTree>
        end
        if <emptyTree>
          "hello"
        else
          <emptyTree>
        end
        <self>.puts(2.+(2))
        <self>.puts(2.+(<emptyTree>::<C <ErrorNode>>))
        if 2.+(<emptyTree>::<C <ErrorNode>>)
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -12,554 +12,36 @@
         f = <emptyTree>::<C T>.let(false, <emptyTree>::<C T>::<C Boolean>)
         <self>.puts(::<Magic>.<build-range>(0, 0, false))
         <self>.puts(::<Magic>.<build-range>(0, <emptyTree>, false))
-        if <emptyTree>::<C <ErrorNode>>
+        if <emptyTree>
           <emptyTree>
         else
           <emptyTree>
         end
-      end
-    end
-
-    if <emptyTree>
-      <self>.puts("hello")
-    else
-      <emptyTree>
-    end
-
-    <self>.puts(::<Magic>.<build-range>(1, 1, true))
-
-    <self>.puts(::<Magic>.<build-range>(1, <emptyTree>, true))
-
-    if <emptyTree>::<C <ErrorNode>>
-      <emptyTree>
-    else
-      <emptyTree>
-    end
-  end
-
-  if <emptyTree>
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(2.+(2))
-
-  <self>.puts(2.+(<emptyTree>::<C <ErrorNode>>))
-
-  if 2.+(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 2.+(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(3.-(3))
-
-  <self>.puts(3.-(<emptyTree>::<C <ErrorNode>>))
-
-  if 3.-(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 3.-(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(4.*(4))
-
-  <self>.puts(4.*(<emptyTree>::<C <ErrorNode>>))
-
-  if 4.*(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 4.*(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(5./(5))
-
-  <self>.puts(5./(<emptyTree>::<C <ErrorNode>>))
-
-  if 5./(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 5./(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(6.%(6))
-
-  <self>.puts(6.%(<emptyTree>::<C <ErrorNode>>))
-
-  if 6.%(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 6.%(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(7.**(7))
-
-  <self>.puts(7.**(<emptyTree>::<C <ErrorNode>>))
-
-  if 7.**(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 7.**(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(8.**(8).-@())
-
-  <self>.puts(8.**(<emptyTree>::<C <ErrorNode>>).-@())
-
-  if 8.**(<emptyTree>::<C <ErrorNode>>).-@()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 8.**(<emptyTree>::<C <ErrorNode>>).-@()
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(9.**(9).+@())
-
-  <self>.puts(9.**(<emptyTree>::<C <ErrorNode>>).+@())
-
-  if 9.**(<emptyTree>::<C <ErrorNode>>).+@()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 9.**(<emptyTree>::<C <ErrorNode>>).+@()
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(-10)
-
-  <self>.puts(<emptyTree>::<C <ErrorNode>>.-@())
-
-  if <emptyTree>::<C <ErrorNode>>.-@()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if <emptyTree>::<C <ErrorNode>>.-@()
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(10)
-
-  <self>.puts(<emptyTree>::<C <ErrorNode>>.+@())
-
-  if <emptyTree>::<C <ErrorNode>>.+@()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if <emptyTree>::<C <ErrorNode>>.+@()
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(10.|(10))
-
-  <self>.puts(10.|(<emptyTree>::<C <ErrorNode>>))
-
-  if 10.|(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 10.|(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(11.^(11))
-
-  <self>.puts(11.^(<emptyTree>::<C <ErrorNode>>))
-
-  if 11.^(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 11.^(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(12.&(12))
-
-  <self>.puts(12.&(<emptyTree>::<C <ErrorNode>>))
-
-  if 12.&(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 12.&(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(13.<=>(13))
-
-  <self>.puts(13.<=>(<emptyTree>::<C <ErrorNode>>))
-
-  if 13.<=>(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 13.<=>(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(14.==(14))
-
-  <self>.puts(14.==(<emptyTree>::<C <ErrorNode>>))
-
-  if 14.==(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 14.==(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(15.===(15))
-
-  <self>.puts(15.===(<emptyTree>::<C <ErrorNode>>))
-
-  if 15.===(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 15.===(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(16.!=(16))
-
-  <self>.puts(16.!=(<emptyTree>::<C <ErrorNode>>))
-
-  if 16.!=(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 16.!=(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(::Regexp.new("17", 0).=~("17"))
-
-  <self>.puts(::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>))
-
-  if ::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if ::Regexp.new("17", 0).=~(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(::Regexp.new("18", 0).!~("eighteen"))
-
-  <self>.puts(::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>))
-
-  if ::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if ::Regexp.new("18", 0).!~(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(<self>.t().!())
-
-  <self>.puts(<emptyTree>::<C <ErrorNode>>.!())
-
-  if <emptyTree>::<C <ErrorNode>>.!()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if <self>.puts("hello").!()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(19.~())
-
-  <self>.puts(<emptyTree>::<C <ErrorNode>>.~())
-
-  if <emptyTree>::<C <ErrorNode>>.~()
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if <emptyTree>::<C <ErrorNode>>.~()
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(20.<<(20))
-
-  <self>.puts(20.<<(<emptyTree>::<C <ErrorNode>>))
-
-  if 20.<<(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 20.<<(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(21.>>(21))
-
-  <self>.puts(21.>>(<emptyTree>::<C <ErrorNode>>))
-
-  if 21.>>(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 21.>>(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(begin
-      &&$2 = <self>.t()
-      if &&$2
-        <self>.t()
-      else
-        &&$2
+        if <emptyTree>
+          "hello"
+        else
+          <emptyTree>
+        end
+        <self>.puts(::<Magic>.<build-range>(1, 1, true))
+        <self>.puts(::<Magic>.<build-range>(1, <emptyTree>, true))
+        if <emptyTree>
+          <emptyTree>
+        else
+          <emptyTree>
+        end
+        if <emptyTree>
+          "hello"
+        else
+          <emptyTree>
+        end
+        <self>.puts(2.+(2))
+        <self>.puts(2.+(<emptyTree>::<C <ErrorNode>>))
+        if 2.+(<emptyTree>::<C <ErrorNode>>)
+          <emptyTree>
+        else
+          <emptyTree>
+        end
       end
-    end)
-
-  <self>.puts(begin
-      &&$3 = <self>.t()
-      if &&$3
-        <emptyTree>::<C <ErrorNode>>
-      else
-        &&$3
-      end
-    end)
-
-  if begin
-      &&$4 = <self>.t()
-      if &&$4
-        <emptyTree>::<C <ErrorNode>>
-      else
-        &&$4
-      end
     end
-    <emptyTree>
-  else
-    <emptyTree>
   end
-
-  if begin
-      &&$5 = <self>.t()
-      if &&$5
-        <emptyTree>::<C <ErrorNode>>
-      else
-        &&$5
-      end
-    end
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(begin
-      ||$6 = <self>.f()
-      if ||$6
-        ||$6
-      else
-        <self>.f()
-      end
-    end)
-
-  <self>.puts(begin
-      ||$7 = <self>.f()
-      if ||$7
-        ||$7
-      else
-        <emptyTree>::<C <ErrorNode>>
-      end
-    end)
-
-  if begin
-      ||$8 = <self>.f()
-      if ||$8
-        ||$8
-      else
-        <emptyTree>::<C <ErrorNode>>
-      end
-    end
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if begin
-      ||$9 = <self>.f()
-      if ||$9
-        ||$9
-      else
-        <emptyTree>::<C <ErrorNode>>
-      end
-    end
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(24.>(24))
-
-  <self>.puts(24.>(<emptyTree>::<C <ErrorNode>>))
-
-  if 24.>(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 24.>(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(25.<(25))
-
-  <self>.puts(25.<(<emptyTree>::<C <ErrorNode>>))
-
-  if 25.<(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 25.<(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(26.>=(26))
-
-  <self>.puts(26.>=(<emptyTree>::<C <ErrorNode>>))
-
-  if 26.>=(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 26.>=(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
-
-  <self>.puts(27.<=(27))
-
-  <self>.puts(27.<=(<emptyTree>::<C <ErrorNode>>))
-
-  if 27.<=(<emptyTree>::<C <ErrorNode>>)
-    <emptyTree>
-  else
-    <emptyTree>
-  end
-
-  if 27.<=(<emptyTree>::<C <ErrorNode>>)
-    <self>.puts("hello")
-  else
-    <emptyTree>
-  end
 end
```

</details>

## module_indent_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class A
  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_1.rb:8: unexpected token "end of file" https://srb.help/2001
     8 |end # error: unexpected token "end of file"
     9 |

test/testdata/parser/error_recovery/module_indent_1.rb:3: Hint: this "module" token might not be properly closed https://srb.help/2003
     3 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_1.rb:8: Matching `end` found here but is not indented as far
     8 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_1.rb:9: expected an `end` to close the `class` statement https://srb.help/2001
     9 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism correctly identifies the missing end but loses the helpful indentation hint that pinpoints the module/end mismatch, making it less diagnostic than Original.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <emptyTree>
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     module <emptyTree>::<C Inner><<C <todo sym>>> < ()
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## module_indent_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class B
  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed
    puts 'hello'
    puts 'hello'

  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_2.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/module_indent_2.rb:3: Hint: this "module" token might not be properly closed https://srb.help/2003
     3 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_2.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_2.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the indentation mismatch issue that Original correctly identifies, only reporting the missing end without the helpful hint about the improperly closed module token.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")

      <self>.puts("hello")
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")

      <self>.puts("hello")

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -4,10 +4,12 @@
       <self>.puts("hello")
 
       <self>.puts("hello")
-    end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## module_indent_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class C
  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_3.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/module_indent_3.rb:3: Hint: this "module" token might not be properly closed https://srb.help/2003
     3 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_3.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_3.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the indentation mismatch issue entirely and incorrectly diagnoses the problem as a missing class end rather than the improperly closed module.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <emptyTree>
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.sig() do ||
        <self>.void()
      end

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,15 +1,15 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C C><<C <todo sym>>> < (::<todo sym>)
     module <emptyTree>::<C Inner><<C <todo sym>>> < ()
-      <emptyTree>
-    end
+      <self>.sig() do ||
+        <self>.void()
+      end
 
-    <self>.sig() do ||
-      <self>.void()
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
-    end
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## module_indent_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class D
  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed
    puts 'hello'

  sig {void}
  def method2
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_4.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/module_indent_4.rb:3: Hint: this "module" token might not be properly closed https://srb.help/2003
     3 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_4.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_4.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the indentation mismatch issue that Original correctly identifies, only reporting the missing end without the helpful hint about the improperly closed module token.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")
    end

    <self>.sig() do ||
      <self>.void()
    end

    def method2<<todo method>>(&<blk>)
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")

      <self>.sig() do ||
        <self>.void()
      end

      def method2<<todo method>>(&<blk>)
        <emptyTree>
      end
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -2,14 +2,16 @@
   class <emptyTree>::<C D><<C <todo sym>>> < (::<todo sym>)
     module <emptyTree>::<C Inner><<C <todo sym>>> < ()
       <self>.puts("hello")
-    end
 
-    <self>.sig() do ||
-      <self>.void()
-    end
+      <self>.sig() do ||
+        <self>.void()
+      end
 
-    def method2<<todo method>>(&<blk>)
-      <emptyTree>
+      def method2<<todo method>>(&<blk>)
+        <emptyTree>
+      end
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## module_indent_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class E
  sig {void}
  def method1
  end

  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_5.rb:9: unexpected token "end of file" https://srb.help/2001
     9 |end # error: unexpected token "end of file"
    10 |

test/testdata/parser/error_recovery/module_indent_5.rb:7: Hint: this "module" token might not be properly closed https://srb.help/2003
     7 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_5.rb:9: Matching `end` found here but is not indented as far
     9 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_5.rb:10: expected an `end` to close the `class` statement https://srb.help/2001
    10 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the indentation mismatch issue entirely and only reports a generic missing end error at EOF, while Original correctly identifies the module/end indentation problem at line 7-9.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <emptyTree>
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C E><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <emptyTree>
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     module <emptyTree>::<C Inner><<C <todo sym>>> < ()
       <emptyTree>
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## module_indent_6.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
class F
  sig {void}
  def method1
  end

  module Inner
# ^^^^^^ error: Hint: this "module" token might not be properly closed
    puts 'hello'
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/module_indent_6.rb:10: unexpected token "end of file" https://srb.help/2001
    10 |end # error: unexpected token "end of file"
    11 |

test/testdata/parser/error_recovery/module_indent_6.rb:7: Hint: this "module" token might not be properly closed https://srb.help/2003
     7 |  module Inner
          ^^^^^^
    test/testdata/parser/error_recovery/module_indent_6.rb:10: Matching `end` found here but is not indented as far
    10 |end # error: unexpected token "end of file"
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/module_indent_6.rb:11: expected an `end` to close the `class` statement https://srb.help/2001
    11 |
        ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses the indentation mismatch issue entirely and only reports a generic missing end error at EOF, while Original correctly identifies the improperly closed module with helpful indentation hints.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C F><<C <todo sym>>> < (::<todo sym>)
    <self>.sig() do ||
      <self>.void()
    end

    def method1<<todo method>>(&<blk>)
      <emptyTree>
    end

    module <emptyTree>::<C Inner><<C <todo sym>>> < ()
      <self>.puts("hello")
    end

    <emptyTree>::<C <ErrorNode>>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -11,5 +11,7 @@
     module <emptyTree>::<C Inner><<C <todo sym>>> < ()
       <self>.puts("hello")
     end
+
+    <emptyTree>::<C <ErrorNode>>
   end
 end
```

</details>

## multiple_stmts.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

def test_method_with_multiple_stmts
  x = 1
  x = ; # error: unexpected token
  y = 1
end

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/multiple_stmts.rb:5: unexpected token ";" https://srb.help/2001
     5 |  x = ; # error: unexpected token
              ^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/multiple_stmts.rb:5: expected an expression after `=` https://srb.help/2001
     5 |  x = ; # error: unexpected token
            ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism provides a more precise error message that directly identifies the missing expression after the assignment operator, with accurate location pointing to the equals sign.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_method_with_multiple_stmts<<todo method>>(&<blk>)
    begin
      x = 1
      x = <emptyTree>::<C <ErrorNode>>
      y = 1
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def test_method_with_multiple_stmts<<todo method>>(&<blk>)
    begin
      x = 1
      x = <emptyTree>::<C <ErrorNode>>
      y = 1
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## numparams.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

[1,2,3].map { |x| _1 }
                # ^^ error: numbered parameters are not allowed when an ordinary parameter is defined

# recover
successful_parse = 10

[1,2,3].map { _1.times { _1 }}
                       # ^^ error: numbered parameter is already used in an outer scope

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/numparams.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |[1,2,3].map { |x| _1 }
                          ^^

test/testdata/parser/error_recovery/numparams.rb:9: numbered parameter is already used in an outer scope https://srb.help/2001
     9 |[1,2,3].map { _1.times { _1 }}
                                 ^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
test/testdata/parser/error_recovery/numparams.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |[1,2,3].map { |x| _1 }
                          ^^

test/testdata/parser/error_recovery/numparams.rb:9: numbered parameter is already used in outer block https://srb.help/2001
     9 |[1,2,3].map { _1.times { _1 }}
                                 ^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism errors are excellent with accurate locations and clear messages that match Original's quality, with only a minor wording difference that is equally understandable.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |_1|
    _1
  end

  successful_parse = 10

  [1, 2, 3].map() do |_1|
    _1.times() do |_1|
      _1
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |x|
    _1
  end

  successful_parse = 10

  [1, 2, 3].map() do |_1|
    _1.times() do ||
      _1
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,12 +1,12 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  [1, 2, 3].map() do |_1|
+  [1, 2, 3].map() do |x|
     _1
   end
 
   successful_parse = 10
 
   [1, 2, 3].map() do |_1|
-    _1.times() do |_1|
+    _1.times() do ||
       _1
     end
   end
```

</details>

## numparams_crash_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

[1,2,3].map { |x| _1 }
                # ^^ error: numbered parameters are not allowed when an ordinary parameter is defined

_1

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_1.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |[1,2,3].map { |x| _1 }
                          ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_1.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |[1,2,3].map { |x| _1 }
                          ^^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism produces identical error output to Original with accurate location and clear message about the numbered parameter conflict.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |_1|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |x|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  [1, 2, 3].map() do |_1|
+  [1, 2, 3].map() do |x|
     _1
   end
 
```

</details>

## numparams_crash_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

f (1) { |x| _1 }
          # ^^ error: numbered parameters are not allowed when an ordinary parameter is defined

_1

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_2.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |f (1) { |x| _1 }
                    ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_2.rb:3: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     3 |f (1) { |x| _1 }
                    ^^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism errors are identical to Original with accurate location and clear message about the numbered parameter conflict.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.f(1) do |_1|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.f(1) do |x|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  <self>.f(1) do |_1|
+  <self>.f(1) do |x|
     _1
   end
 
```

</details>

## numparams_crash_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

foo {&:bar}
  # ^^^^^^^ error: block pass should not be enclosed in curly braces

_1

```

</details>

<details>
<summary>Original errors (1) | Autocorrects: 1</summary>

```
test/testdata/parser/error_recovery/numparams_crash_3.rb:3: block pass should not be enclosed in curly braces https://srb.help/2001
     3 |foo {&:bar}
            ^^^^^^^
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/numparams_crash_3.rb:3: Replace with `(&:bar)`
     3 |foo {&:bar}
            ^^^^^^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_3.rb:3: unexpected '&', ignoring it https://srb.help/2001
     3 |foo {&:bar}
             ^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism reports a different and less accurate error about the ampersand being unexpected, missing the actual issue about block pass syntax in curly braces that Original correctly identifies.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.foo() do ||
    :bar
  end

  <self>._1()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.foo() do ||
    begin
      <emptyTree>::<C <ErrorNode>>
      :bar
    end
  end

  <self>._1()
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,6 +1,9 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   <self>.foo() do ||
-    :bar
+    begin
+      <emptyTree>::<C <ErrorNode>>
+      :bar
+    end
   end
 
   <self>._1()
```

</details>

## numparams_crash_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

-> { _1 }

_1

```

</details>

<details>
<summary>Original errors (0)</summary>

```
No errors! Great job.
```

</details>

<details>
<summary>Prism errors (0)</summary>

```
No errors! Great job.
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Both parsers incorrectly accept invalid code where _1 is used outside a numbered parameter context, which should be a syntax error.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <emptyTree>::<C Kernel>.lambda() do |_1|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <emptyTree>::<C Kernel>.lambda() do |_1|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Desugar tree diff 🟢</summary>

```diff
Trees are identical
```

</details>

## numparams_crash_5.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

[1,2,3].map do |x|
  _1 # error: numbered parameters are not allowed when an ordinary parameter is defined
end

_1

```

</details>

<details>
<summary>Original errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_5.rb:4: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     4 |  _1 # error: numbered parameters are not allowed when an ordinary parameter is defined
          ^^
Errors: 1
```

</details>

<details>
<summary>Prism errors (1)</summary>

```
test/testdata/parser/error_recovery/numparams_crash_5.rb:4: numbered parameters are not allowed when an ordinary parameter is defined https://srb.help/2001
     4 |  _1 # error: numbered parameters are not allowed when an ordinary parameter is defined
          ^^
Errors: 1
```

</details>

<details>
<summary>LLM quality assessment 🟢</summary>

Prism produces identical error output to Original, correctly identifying the numbered parameter error at the exact location with the same clear message.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |_1|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  [1, 2, 3].map() do |x|
    _1
  end

  <self>._1()
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,5 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
-  [1, 2, 3].map() do |_1|
+  [1, 2, 3].map() do |x|
     _1
   end
 
```

</details>

## other_missing_end.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

class A
  def test1
    puts 'before'
    x do # error: Hint: this "do" token might not be properly closed
    puts 'after'
  end

  def test2
    puts 'before'
    x y do # error: Hint: this kDO_BLOCK token might not be properly closed
    puts 'after'
  end

  def test3
    puts 'before'
    -> do # error: Hint: this kDO_LAMBDA token might not be properly closed
    puts 'after'
  end

  def test4
    puts 'before'
    while x do # error: Hint: this "while" token might not be properly closed
    puts 'after'
  end

  def test5
    puts 'before'
    begin # error: Hint: this "begin" token might not be properly closed
    puts 'after'
  end

  def test6
    puts 'before'
    unless nil # error: Hint: this "unless" token might not be properly closed
    puts 'after'
  end

  def test7
    puts 'before'
    while nil # error: Hint: this "while" token might not be properly closed
    puts 'after'
  end

  def test8
    puts 'before'
    until nil # error: Hint: this "until" token might not be properly closed
    puts 'after'
  end

  def test9
    puts 'before'
    unless # error: unexpected token "unless"
  end

  def test10
    puts 'before'
    while # error: unexpected token "while"
  end

  def test11
    puts 'before'
    until # error: unexpected token "until"
  end
end # error: unexpected token "end of file"

```

</details>

<details>
<summary>Original errors (12)</summary>

```
test/testdata/parser/error_recovery/other_missing_end.rb:54: unexpected token "unless" https://srb.help/2001
    54 |    unless # error: unexpected token "unless"
            ^^^^^^

test/testdata/parser/error_recovery/other_missing_end.rb:59: unexpected token "while" https://srb.help/2001
    59 |    while # error: unexpected token "while"
            ^^^^^

test/testdata/parser/error_recovery/other_missing_end.rb:64: unexpected token "until" https://srb.help/2001
    64 |    until # error: unexpected token "until"
            ^^^^^

test/testdata/parser/error_recovery/other_missing_end.rb:66: unexpected token "end of file" https://srb.help/2001
    66 |end # error: unexpected token "end of file"
    67 |

test/testdata/parser/error_recovery/other_missing_end.rb:6: Hint: this "do" token might not be properly closed https://srb.help/2003
     6 |    x do # error: Hint: this "do" token might not be properly closed
              ^^
    test/testdata/parser/error_recovery/other_missing_end.rb:8: Matching `end` found here but is not indented as far
     8 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:12: Hint: this kDO_BLOCK token might not be properly closed https://srb.help/2003
    12 |    x y do # error: Hint: this kDO_BLOCK token might not be properly closed
                ^^
    test/testdata/parser/error_recovery/other_missing_end.rb:14: Matching `end` found here but is not indented as far
    14 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:18: Hint: this kDO_LAMBDA token might not be properly closed https://srb.help/2003
    18 |    -> do # error: Hint: this kDO_LAMBDA token might not be properly closed
               ^^
    test/testdata/parser/error_recovery/other_missing_end.rb:20: Matching `end` found here but is not indented as far
    20 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:24: Hint: this "while" token might not be properly closed https://srb.help/2003
    24 |    while x do # error: Hint: this "while" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/other_missing_end.rb:26: Matching `end` found here but is not indented as far
    26 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:30: Hint: this "begin" token might not be properly closed https://srb.help/2003
    30 |    begin # error: Hint: this "begin" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/other_missing_end.rb:32: Matching `end` found here but is not indented as far
    32 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:36: Hint: this "unless" token might not be properly closed https://srb.help/2003
    36 |    unless nil # error: Hint: this "unless" token might not be properly closed
            ^^^^^^
    test/testdata/parser/error_recovery/other_missing_end.rb:38: Matching `end` found here but is not indented as far
    38 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:42: Hint: this "while" token might not be properly closed https://srb.help/2003
    42 |    while nil # error: Hint: this "while" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/other_missing_end.rb:44: Matching `end` found here but is not indented as far
    44 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:48: Hint: this "until" token might not be properly closed https://srb.help/2003
    48 |    until nil # error: Hint: this "until" token might not be properly closed
            ^^^^^
    test/testdata/parser/error_recovery/other_missing_end.rb:50: Matching `end` found here but is not indented as far
    50 |  end
          ^^^
Errors: 12
```

</details>

<details>
<summary>Prism errors (12)</summary>

```
test/testdata/parser/error_recovery/other_missing_end.rb:54: expected a predicate expression for the `unless` statement https://srb.help/2001
    54 |    unless # error: unexpected token "unless"
            ^^^^^^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected `then` or `;` or '\n' https://srb.help/2001
    55 |  end
          ^^^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `def` statement https://srb.help/2001
    55 |  end
             ^

test/testdata/parser/error_recovery/other_missing_end.rb:55: expected an `end` to close the `class` statement https://srb.help/2001
    55 |  end
             ^
Errors: 12
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism misses all the helpful indentation hints for unclosed blocks and produces 10 repetitive unhelpful errors at line 55 instead of identifying the actual missing end locations throughout the file.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x() do ||
          <self>.puts("after")
        end
      end
    end

    def test2<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x(<self>.y()) do ||
          <self>.puts("after")
        end
      end
    end

    def test3<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <emptyTree>::<C Kernel>.lambda() do ||
          <self>.puts("after")
        end
      end
    end

    def test4<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        while <self>.x()
          <self>.puts("after")
        end
      end
    end

    def test5<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.puts("after")
      end
    end

    def test6<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if nil
          <emptyTree>
        else
          <self>.puts("after")
        end
      end
    end

    def test7<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        while nil
          <self>.puts("after")
        end
      end
    end

    def test8<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        while nil.!()
          <self>.puts("after")
        end
      end
    end

    def test9<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        if <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        else
          <emptyTree>
        end
      end
    end

    def test10<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        while <emptyTree>::<C <ErrorNode>>
          <emptyTree>
        end
      end
    end

    def test11<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        while <emptyTree>::<C <ErrorNode>>.!()
          <emptyTree>
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test1<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        <self>.x() do ||
          <self>.puts("after")
        end
        def test2<<todo method>>(&<blk>)
          begin
            <self>.puts("before")
            <self>.x(<self>.y()) do ||
              <self>.puts("after")
            end
            def test3<<todo method>>(&<blk>)
              begin
                <self>.puts("before")
                <emptyTree>::<C Kernel>.lambda() do ||
                  <self>.puts("after")
                end
                def test4<<todo method>>(&<blk>)
                  begin
                    <self>.puts("before")
                    while <self>.x()
                      <self>.puts("after")
                    end
                    def test5<<todo method>>(&<blk>)
                      begin
                        <self>.puts("before")
                        <self>.puts("after")
                        def test6<<todo method>>(&<blk>)
                          begin
                            <self>.puts("before")
                            if nil
                              <emptyTree>
                            else
                              <self>.puts("after")
                            end
                            def test7<<todo method>>(&<blk>)
                              begin
                                <self>.puts("before")
                                while nil
                                  <self>.puts("after")
                                end
                                def test8<<todo method>>(&<blk>)
                                  begin
                                    <self>.puts("before")
                                    while nil.!()
                                      <self>.puts("after")
                                    end
                                    def test9<<todo method>>(&<blk>)
                                      begin
                                        <self>.puts("before")
                                        if <emptyTree>::<C <ErrorNode>>
                                          <emptyTree>
                                        else
                                          <emptyTree>
                                        end
                                      end
                                    end
                                  end
                                end
                              end
                            end
                          end
                        end
                      end
                    end
                  end
                end
              end
            end
          end
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -6,99 +6,73 @@
         <self>.x() do ||
           <self>.puts("after")
         end
-      end
-    end
-
-    def test2<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <self>.x(<self>.y()) do ||
-          <self>.puts("after")
+        def test2<<todo method>>(&<blk>)
+          begin
+            <self>.puts("before")
+            <self>.x(<self>.y()) do ||
+              <self>.puts("after")
+            end
+            def test3<<todo method>>(&<blk>)
+              begin
+                <self>.puts("before")
+                <emptyTree>::<C Kernel>.lambda() do ||
+                  <self>.puts("after")
+                end
+                def test4<<todo method>>(&<blk>)
+                  begin
+                    <self>.puts("before")
+                    while <self>.x()
+                      <self>.puts("after")
+                    end
+                    def test5<<todo method>>(&<blk>)
+                      begin
+                        <self>.puts("before")
+                        <self>.puts("after")
+                        def test6<<todo method>>(&<blk>)
+                          begin
+                            <self>.puts("before")
+                            if nil
+                              <emptyTree>
+                            else
+                              <self>.puts("after")
+                            end
+                            def test7<<todo method>>(&<blk>)
+                              begin
+                                <self>.puts("before")
+                                while nil
+                                  <self>.puts("after")
+                                end
+                                def test8<<todo method>>(&<blk>)
+                                  begin
+                                    <self>.puts("before")
+                                    while nil.!()
+                                      <self>.puts("after")
+                                    end
+                                    def test9<<todo method>>(&<blk>)
+                                      begin
+                                        <self>.puts("before")
+                                        if <emptyTree>::<C <ErrorNode>>
+                                          <emptyTree>
+                                        else
+                                          <emptyTree>
+                                        end
+                                      end
+                                    end
+                                  end
+                                end
+                              end
+                            end
+                          end
+                        end
+                      end
+                    end
+                  end
+                end
+              end
+            end
+          end
         end
       end
     end
-
-    def test3<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <emptyTree>::<C Kernel>.lambda() do ||
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test4<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        while <self>.x()
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test5<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        <self>.puts("after")
-      end
-    end
-
-    def test6<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        if nil
-          <emptyTree>
-        else
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test7<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        while nil
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test8<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        while nil.!()
-          <self>.puts("after")
-        end
-      end
-    end
-
-    def test9<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        if <emptyTree>::<C <ErrorNode>>
-          <emptyTree>
-        else
-          <emptyTree>
-        end
-      end
-    end
-
-    def test10<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        while <emptyTree>::<C <ErrorNode>>
-          <emptyTree>
-        end
-      end
-    end
-
-    def test11<<todo method>>(&<blk>)
-      begin
-        <self>.puts("before")
-        while <emptyTree>::<C <ErrorNode>>.!()
-          <emptyTree>
-        end
-      end
-    end
   end
 end
```

</details>

## pos_after_kw_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false
sig do
  params(
    build_group: String,
    type
  # ^^^^ error: positional arg "type" after keyword arg
  # ^^^^ error: Malformed type declaration. Unknown type syntax. Expected a ClassName or T.<func>
  # ^^^^ error: Unknown parameter name `type`
    filter: T.nilable(String),
  )
  .returns(T::Array[String])
end
def call(build_group, filter:)
end

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/pos_after_kw_1.rb:5: positional arg "type" after keyword arg https://srb.help/2001
     5 |    type
            ^^^^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:5: Malformed type declaration. Unknown type syntax. Expected a ClassName or T.<func> https://srb.help/5004
     5 |    type
            ^^^^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:5: Unknown parameter name `type` https://srb.help/5003
     5 |    type
            ^^^^
    test/testdata/parser/error_recovery/pos_after_kw_1.rb:13: Parameter not in method definition here
    13 |def call(build_group, filter:)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (12)</summary>

```
test/testdata/parser/error_recovery/pos_after_kw_1.rb:5: expected a `=>` between the hash key and value https://srb.help/2001
     5 |    type
                ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:5: unexpected newline; expected a value in the hash literal https://srb.help/2001
     5 |    type
     6 |  # ^^^^ error: positional arg "type" after keyword arg

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected local variable or method; expected a `)` to close the arguments https://srb.help/2001
     9 |    filter: T.nilable(String),
            ^^^^^^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected local variable or method, expecting end-of-input https://srb.help/2001
     9 |    filter: T.nilable(String),
            ^^^^^^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected ':', expecting end-of-input https://srb.help/2001
     9 |    filter: T.nilable(String),
                  ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected ':', ignoring it https://srb.help/2001
     9 |    filter: T.nilable(String),
                  ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected ',', expecting end-of-input https://srb.help/2001
     9 |    filter: T.nilable(String),
                                     ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:9: unexpected ',', ignoring it https://srb.help/2001
     9 |    filter: T.nilable(String),
                                     ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:10: unexpected ')', ignoring it https://srb.help/2001
    10 |  )
          ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:11: unexpected '.', ignoring it https://srb.help/2001
    11 |  .returns(T::Array[String])
          ^

test/testdata/parser/error_recovery/pos_after_kw_1.rb:2: Malformed `sig`: Signature blocks must contain a single statement https://srb.help/5069
     2 |sig do
     3 |  params(
     4 |    build_group: String,
     5 |    type
     6 |  # ^^^^ error: positional arg "type" after keyword arg
       |...
     8 |  # ^^^^ error: Unknown parameter name `type`
     9 |    filter: T.nilable(String),
    10 |  )
    11 |  .returns(T::Array[String])
    12 |end

test/testdata/parser/error_recovery/pos_after_kw_1.rb:2: Malformed `sig`: No return type specified. Specify one with .returns() https://srb.help/5003
     2 |sig do
     3 |  params(
     4 |    build_group: String,
     5 |    type
     6 |  # ^^^^ error: positional arg "type" after keyword arg
       |...
     8 |  # ^^^^ error: Unknown parameter name `type`
     9 |    filter: T.nilable(String),
    10 |  )
    11 |  .returns(T::Array[String])
    12 |end
Errors: 12
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism completely misparses the sig block structure, generating 12 cascading syntax errors instead of the 3 accurate semantic errors about positional-after-keyword argument ordering that Original correctly identifies.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.sig() do ||
    <self>.params(:build_group, <emptyTree>::<C String>, :type, <self>.type(), :filter, <emptyTree>::<C T>.nilable(<emptyTree>::<C String>)).returns(<emptyTree>::<C T>::<C Array>.[](<emptyTree>::<C String>))
  end

  def call<<todo method>>(build_group, filter:, &<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  <self>.sig() do ||
    begin
      <self>.params({:build_group => <emptyTree>::<C String>, <self>.type() => <emptyTree>::<C <ErrorNode>>})
      <self>.filter()
      <emptyTree>::<C <ErrorNode>>
      <emptyTree>::<C T>.nilable(<emptyTree>::<C String>)
      <emptyTree>::<C <ErrorNode>>
      <emptyTree>::<C <ErrorNode>>
      <emptyTree>::<C <ErrorNode>>
      <self>.returns(<emptyTree>::<C T>::<C Array>.[](<emptyTree>::<C String>))
    end
  end

  def call<<todo method>>(build_group, filter:, &<blk>)
    <emptyTree>
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,6 +1,15 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   <self>.sig() do ||
-    <self>.params(:build_group, <emptyTree>::<C String>, :type, <self>.type(), :filter, <emptyTree>::<C T>.nilable(<emptyTree>::<C String>)).returns(<emptyTree>::<C T>::<C Array>.[](<emptyTree>::<C String>))
+    begin
+      <self>.params({:build_group => <emptyTree>::<C String>, <self>.type() => <emptyTree>::<C <ErrorNode>>})
+      <self>.filter()
+      <emptyTree>::<C <ErrorNode>>
+      <emptyTree>::<C T>.nilable(<emptyTree>::<C String>)
+      <emptyTree>::<C <ErrorNode>>
+      <emptyTree>::<C <ErrorNode>>
+      <emptyTree>::<C <ErrorNode>>
+      <self>.returns(<emptyTree>::<C T>::<C Array>.[](<emptyTree>::<C String>))
+    end
   end
 
   def call<<todo method>>(build_group, filter:, &<blk>)
```

</details>

## send_after_end.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

# We don't recover from this code well as we'd like: ideally, this should parse
# the method call to `x.if` inside the `def foo`, but instead it puts the
# method call outside the MethodDef node.
#
# This is part of a larger problem that a single mismatched `end` can really
# screw over our ability to recover (see error_recovery_if_no_end.rb).
#
# This code is empirically rare (doesn't accur at all at Stripe), but it's
# unclear how often code like this comes up when people are making edits.

def foo
  x.end

  x.
    end # error: unexpected token

  x.if
end # error: unexpected token

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/send_after_end.rb:17: unexpected token "end" https://srb.help/2001
    17 |    end # error: unexpected token
            ^^^

test/testdata/parser/error_recovery/send_after_end.rb:20: unexpected token "end" https://srb.help/2001
    20 |end # error: unexpected token
        ^^^
Errors: 2
```

</details>

<details>
<summary>Prism errors (0)</summary>

```
No errors! Great job.
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism fails to detect syntax errors that Original correctly identifies, missing both the unexpected end token on line 17 and the mismatched end on line 20.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    begin
      <self>.x().end()
      <self>.x().<method-name-missing>()
    end
  end

  <self>.x().if()
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def foo<<todo method>>(&<blk>)
    begin
      <self>.x().end()
      <self>.x().end()
      <self>.x().if()
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -2,9 +2,8 @@
   def foo<<todo method>>(&<blk>)
     begin
       <self>.x().end()
-      <self>.x().<method-name-missing>()
+      <self>.x().end()
+      <self>.x().if()
     end
   end
-
-  <self>.x().if()
 end
```

</details>

## singleton_method_literal.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

def (10).test()
   # ^^ error: cannot define a singleton method for a literal
   # ^^ error: `def EXPRESSION.method` is only supported for `def self.method`
  [1,2,3].each do |x|
    x
  end
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/singleton_method_literal.rb:3: cannot define a singleton method for a literal https://srb.help/2001
     3 |def (10).test()
             ^^

test/testdata/parser/error_recovery/singleton_method_literal.rb:3: `def EXPRESSION.method` is only supported for `def self.method` https://srb.help/3001
     3 |def (10).test()
             ^^
  Note:
    When it's imperative to define a singleton method on an object,
    use `EXPRESSION.define_singleton_method(:method) { ... }` instead.
    The method will NOT be visible to Sorbet.
Errors: 2
```

</details>

<details>
<summary>Prism errors (2)</summary>

```
Exception::raise() msg="parser/prism/Helpers.h:202 enforced condition anyNode == nullptr || (((enum pm_node_type) (anyNode)->type) == (PrismNodeTypeHelper<PrismNode>::TypeID)) has failed: Failed to cast a Prism AST Node. Expected PM_STATEMENTS_NODE (#140), but got PM_INTEGER_NODE (#PM_INTEGER_NODE)."

Backtrace:
pm_encoding_windows_874_table (in sorbet) + 4652
std::__1::piecewise_construct (in sorbet) + 92
descriptor_table_protodef_google_2fprotobuf_2fsource_5fcontext_2eproto (in sorbet) + 124
context_terminators (in sorbet) + 872
typeinfo name for void sorbet::Parallel::iterate<sorbet::ast::ParsedFile, void sorbet::packager::(anonymous namespace)::packageRunCore<(sorbet::packager::(anonymous namespace)::PackagerMode)1>(sorbet::core::GlobalState&, sorbet::WorkerPool&, absl::lts_20240722::Span<sorbet::ast::ParsedFile>)::'lambda'(auto&)>(sorbet::WorkerPool&, std::__1::basic_string_view<char, std::__1::char_traits<char>>, absl::lts_20240722::Span<auto>, void sorbet::packager::(anonymous namespace)::packageRunCore<(sorbet::packager::(anonymous namespace)::PackagerMode)1>(sorbet::core::GlobalState&, sorbet::WorkerPool&, absl::lts_20240722::Span<sorbet::ast::ParsedFile>)::'lambda'(auto&))::'lambda'() (in sorbet) + 310
std::__1::piecewise_construct (in sorbet) + 732
GCC_except_table9 (in sorbet) + 24
sorbet::ast::RescueCase::toStringWithTabs(sorbet::core::GlobalState const&, int) const (in sorbet) + 1056
sorbet::ast::If::toStringWithTabs(sorbet::core::GlobalState const&, int) const (in sorbet) + 256
sorbet::ast::Cast::toStringWithTabs(sorbet::core::GlobalState const&, int) const (in sorbet) + 40
std::__1::__compressed_pair<std::__1::__state<char>**, std::__1::allocator<std::__1::__state<char>*>&>::__compressed_pair[abi:v160005]<std::nullptr_t, std::__1::allocator<std::__1::__state<char>*>&>(std::nullptr_t&&, std::__1::allocator<std::__1::__state<char>*>&) (in sorbet) + 44
std::__1::__tree_node_base<void*>*& std::__1::__tree<std::__1::pair<unsigned int, unsigned int>, std::__1::less<std::__1::pair<unsigned int, unsigned int>>, std::__1::allocator<std::__1::pair<unsigned int, unsigned int>>>::__find_equal<std::__1::pair<unsigned int, unsigned int>>(std::__1::__tree_end_node<std::__1::__tree_node_base<void*>*>*&, std::__1::pair<unsigned int, unsigned int> const&) (in sorbet) + 196
start (in dyld) + 7184

test/testdata/parser/error_recovery/singleton_method_literal.rb: Exception parsing file: `test/testdata/parser/error_recovery/singleton_method_literal.rb` (backtrace is above) https://srb.help/1001

test/testdata/parser/error_recovery/singleton_method_literal.rb:3: cannot define singleton method for literals https://srb.help/2001
     3 |def (10).test()
             ^^
Errors: 2
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism crashes with an internal parser exception instead of gracefully handling the singleton method literal error like Original does.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  def self.test<<todo method>>(&<blk>)
    [1, 2, 3].each() do |x|
      x
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```

```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
One or both trees are empty
```

</details>

## unmatched_block_args_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module Opus::Log
  def self.info; end
end

class A
  def test_no_args
    1.times do | # error: unmatched "|" in block argument list
    end
    1.times do | # error: unmatched "|" in block argument list
      puts 'hello'
    end
    1.times do | # error: unmatched "|" in block argument list
      puts('hello')
    end
    1.times do | # error: unmatched "|" in block argument list
      x = nil
    end
    1.times do | # error: unmatched "|" in block argument list
      Opus::Log.info
    end
  end

  def test_one_arg
    1.times do |x # error: unmatched "|" in block argument list
    end
    1.times do |x # error: unmatched "|" in block argument list
      puts 'hello'
    end
    1.times do |x # error: unmatched "|" in block argument list
      puts('hello')
    end
    1.times do |x # error: unmatched "|" in block argument list
      x = nil
    end
    1.times do |x # error: unmatched "|" in block argument list
      Opus::Log.info
    end
  end

  def test_one_arg_comma
    1.times do |x, # error: unmatched "|" in block argument list
    end
    # 1.times do |x,
    #   puts 'hello'
    # end
    1.times do |x, # error: unmatched "|" in block argument list
      puts('hello') # error: unexpected token tNL
    end
    1.times do |x, # error: unmatched "|" in block argument list
      x = nil
    end
    1.times do |x, # error: unmatched "|" in block argument list
      Opus::Log.info # error: unexpected token tNL
    end
  end

  def test_two_args
    1.times do |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
    end
    1.times do |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      puts 'hello'
    end
    1.times do |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      puts('hello')
    end
    1.times do |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      x = nil
    end
    1.times do |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      Opus::Log.info
    end
  end
end

```

</details>

<details>
<summary>Original errors (26)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_1.rb:9: unmatched "|" in block argument list https://srb.help/2001
     9 |    1.times do | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:11: unmatched "|" in block argument list https://srb.help/2001
    11 |    1.times do | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:14: unmatched "|" in block argument list https://srb.help/2001
    14 |    1.times do | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:17: unmatched "|" in block argument list https://srb.help/2001
    17 |    1.times do | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:20: unmatched "|" in block argument list https://srb.help/2001
    20 |    1.times do | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:26: unmatched "|" in block argument list https://srb.help/2001
    26 |    1.times do |x # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:28: unmatched "|" in block argument list https://srb.help/2001
    28 |    1.times do |x # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:31: unmatched "|" in block argument list https://srb.help/2001
    31 |    1.times do |x # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:34: unmatched "|" in block argument list https://srb.help/2001
    34 |    1.times do |x # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:37: unmatched "|" in block argument list https://srb.help/2001
    37 |    1.times do |x # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:43: unmatched "|" in block argument list https://srb.help/2001
    43 |    1.times do |x, # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:48: unmatched "|" in block argument list https://srb.help/2001
    48 |    1.times do |x, # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:49: unexpected token tNL https://srb.help/2001
    49 |      puts('hello') # error: unexpected token tNL
    50 |    end

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:51: unmatched "|" in block argument list https://srb.help/2001
    51 |    1.times do |x, # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:54: unmatched "|" in block argument list https://srb.help/2001
    54 |    1.times do |x, # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:55: unexpected token tNL https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
    56 |    end

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:60: unmatched "|" in block argument list https://srb.help/2001
    60 |    1.times do |x, y # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:61: unexpected token tNL https://srb.help/2001
    61 |      # error: unexpected token tNL
    62 |    end

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:63: unmatched "|" in block argument list https://srb.help/2001
    63 |    1.times do |x, y # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:64: unexpected token tNL https://srb.help/2001
    64 |      # error: unexpected token tNL
    65 |      puts 'hello'

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:67: unmatched "|" in block argument list https://srb.help/2001
    67 |    1.times do |x, y # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:68: unexpected token tNL https://srb.help/2001
    68 |      # error: unexpected token tNL
    69 |      puts('hello')

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:71: unmatched "|" in block argument list https://srb.help/2001
    71 |    1.times do |x, y # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:72: unexpected token tNL https://srb.help/2001
    72 |      # error: unexpected token tNL
    73 |      x = nil

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:75: unmatched "|" in block argument list https://srb.help/2001
    75 |    1.times do |x, y # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:76: unexpected token tNL https://srb.help/2001
    76 |      # error: unexpected token tNL
    77 |      Opus::Log.info
Errors: 26
```

</details>

<details>
<summary>Prism errors (26)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_1.rb:9: expected the block parameters to end with `|` https://srb.help/2001
     9 |    1.times do | # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:12: expected the block parameters to end with `|` https://srb.help/2001
    12 |      puts 'hello'
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:15: expected the block parameters to end with `|` https://srb.help/2001
    15 |      puts('hello')
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:19: expected the block parameters to end with `|` https://srb.help/2001
    19 |    end
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:21: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    21 |      Opus::Log.info
              ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:21: expected the block parameters to end with `|` https://srb.help/2001
    21 |      Opus::Log.info
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:21: unexpected '::', ignoring it https://srb.help/2001
    21 |      Opus::Log.info
                  ^^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:27: expected the block parameters to end with `|` https://srb.help/2001
    27 |    end
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:29: expected the block parameters to end with `|` https://srb.help/2001
    29 |      puts 'hello'
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:32: expected the block parameters to end with `|` https://srb.help/2001
    32 |      puts('hello')
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:35: expected the block parameters to end with `|` https://srb.help/2001
    35 |      x = nil
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:38: expected the block parameters to end with `|` https://srb.help/2001
    38 |      Opus::Log.info
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:43: expected the block parameters to end with `|` https://srb.help/2001
    43 |    1.times do |x, # error: unmatched "|" in block argument list
                          ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:49: expected the block parameters to end with `|` https://srb.help/2001
    49 |      puts('hello') # error: unexpected token tNL
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:52: duplicated argument name https://srb.help/2001
    52 |      x = nil
              ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:53: expected the block parameters to end with `|` https://srb.help/2001
    53 |    end
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:55: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
              ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:55: expected the block parameters to end with `|` https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:55: unexpected '::', ignoring it https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
                  ^^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:61: expected the block parameters to end with `|` https://srb.help/2001
    61 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:64: expected the block parameters to end with `|` https://srb.help/2001
    64 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:68: expected the block parameters to end with `|` https://srb.help/2001
    68 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:72: expected the block parameters to end with `|` https://srb.help/2001
    72 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:76: expected the block parameters to end with `|` https://srb.help/2001
    76 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:21: Unable to resolve constant `Log` https://srb.help/5002
    21 |      Opus::Log.info
                    ^^^

test/testdata/parser/error_recovery/unmatched_block_args_1.rb:55: Unable to resolve constant `Log` https://srb.help/5002
    55 |      Opus::Log.info # error: unexpected token tNL
                    ^^^
Errors: 26
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism produces confusing errors with inaccurate locations pointing to wrong lines and generates spurious errors about constants and duplicate arguments that don't exist in the original parser.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do ||
          x
        end
        1.times() do ||
          begin
            x
            <self>.puts("hello")
          end
        end
        1.times() do ||
          begin
            x
            <self>.puts("hello")
          end
        end
        1.times() do ||
          begin
            x
            x = nil
          end
        end
        1.times() do ||
          begin
            x
            <emptyTree>::<C Opus>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          begin
            <assignTemp>$2 = nil
            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
            x = <assignTemp>$3.[](0)
            x = <assignTemp>$3.[](1)
            <assignTemp>$2
          end
        end
        1.times() do ||
          <emptyTree>
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |x = nil|
          <emptyTree>
        end
        1.times() do |Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do |x|
          <emptyTree>
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          x = nil
        end
        1.times() do |x|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do |x, *<restargs>|
          <emptyTree>
        end
        1.times() do |x, puts|
          "hello"
        end
        1.times() do |x, x = nil|
          <emptyTree>
        end
        1.times() do |x, Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      begin
        1.times() do |x, y|
          <emptyTree>
        end
        1.times() do |x, y|
          <self>.puts("hello")
        end
        1.times() do |x, y|
          <self>.puts("hello")
        end
        1.times() do |x, y|
          x = nil
        end
        1.times() do |x, y|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -11,91 +11,79 @@
         1.times() do ||
           <emptyTree>
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          x = nil
+        1.times() do |x = nil|
+          <emptyTree>
         end
-        1.times() do ||
-          <emptyTree>::<C Opus>::<C Log>.info()
+        1.times() do |Opus|
+          begin
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
+          end
         end
       end
     end
 
     def test_one_arg<<todo method>>(&<blk>)
       begin
-        1.times() do ||
-          x
+        1.times() do |x|
+          <emptyTree>
         end
-        1.times() do ||
-          begin
-            x
-            <self>.puts("hello")
-          end
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          begin
-            x
-            <self>.puts("hello")
-          end
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          begin
-            x
-            x = nil
-          end
+        1.times() do |x|
+          x = nil
         end
-        1.times() do ||
-          begin
-            x
-            <emptyTree>::<C Opus>::<C Log>.info()
-          end
+        1.times() do |x|
+          <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
     end
 
     def test_one_arg_comma<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, *<restargs>|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, puts|
+          "hello"
+        end
+        1.times() do |x, x = nil|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, Opus|
           begin
-            <assignTemp>$2 = nil
-            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
-            x = <assignTemp>$3.[](0)
-            x = <assignTemp>$3.[](1)
-            <assignTemp>$2
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
           end
         end
-        1.times() do ||
-          <emptyTree>
-        end
       end
     end
 
     def test_two_args<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, y|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, y|
           <self>.puts("hello")
         end
-        1.times() do ||
+        1.times() do |x, y|
           <self>.puts("hello")
         end
-        1.times() do ||
+        1.times() do |x, y|
           x = nil
         end
-        1.times() do ||
+        1.times() do |x, y|
           <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
```

</details>

## unmatched_block_args_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module Opus::Log
  def self.info; end
end

class A
  def test_no_args
    1.times { | # error: unmatched "|" in block argument list
    }
    1.times { | # error: unmatched "|" in block argument list
      puts 'hello'
    }
    1.times { | # error: unmatched "|" in block argument list
      puts('hello')
    }
    1.times { | # error: unmatched "|" in block argument list
      x = nil
    }
    1.times { | # error: unmatched "|" in block argument list
      Opus::Log.info
    }
  end

  def test_one_arg
    1.times { |x # error: unmatched "|" in block argument list
    }
    1.times { |x # error: unmatched "|" in block argument list
      puts 'hello'
    }
    1.times { |x # error: unmatched "|" in block argument list
      puts('hello')
    }
    1.times { |x # error: unmatched "|" in block argument list
      x = nil
    }
    1.times { |x # error: unmatched "|" in block argument list
      Opus::Log.info
    }
  end

  def test_one_arg_comma
    1.times { |x, # error: unmatched "|" in block argument list
    }
    1.times { |x, # error: unmatched "|" in block argument list
      puts 'hello' # error: unexpected token tSTRING
    }
    1.times { |x, # error: unmatched "|" in block argument list
      puts('hello') # error: unexpected token tNL
    }
    1.times { |x, # error: unmatched "|" in block argument list
      x = nil
    }
    1.times { |x, # error: unmatched "|" in block argument list
      Opus::Log.info # error: unexpected token tNL
    }
  end

  def test_two_args
    1.times { |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
    }
    1.times { |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      puts 'hello'
    }
    1.times { |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      puts('hello')
    }
    1.times { |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      x = nil
    }
    1.times { |x, y # error: unmatched "|" in block argument list
      # error: unexpected token tNL
      Opus::Log.info
    }
  end
end

```

</details>

<details>
<summary>Original errors (28)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_2.rb:9: unmatched "|" in block argument list https://srb.help/2001
     9 |    1.times { | # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:11: unmatched "|" in block argument list https://srb.help/2001
    11 |    1.times { | # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:14: unmatched "|" in block argument list https://srb.help/2001
    14 |    1.times { | # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:17: unmatched "|" in block argument list https://srb.help/2001
    17 |    1.times { | # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:20: unmatched "|" in block argument list https://srb.help/2001
    20 |    1.times { | # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:26: unmatched "|" in block argument list https://srb.help/2001
    26 |    1.times { |x # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:28: unmatched "|" in block argument list https://srb.help/2001
    28 |    1.times { |x # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:31: unmatched "|" in block argument list https://srb.help/2001
    31 |    1.times { |x # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:34: unmatched "|" in block argument list https://srb.help/2001
    34 |    1.times { |x # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:37: unmatched "|" in block argument list https://srb.help/2001
    37 |    1.times { |x # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:43: unmatched "|" in block argument list https://srb.help/2001
    43 |    1.times { |x, # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:45: unmatched "|" in block argument list https://srb.help/2001
    45 |    1.times { |x, # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:46: unexpected token tSTRING https://srb.help/2001
    46 |      puts 'hello' # error: unexpected token tSTRING
                   ^^^^^^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:48: unmatched "|" in block argument list https://srb.help/2001
    48 |    1.times { |x, # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:49: unexpected token tNL https://srb.help/2001
    49 |      puts('hello') # error: unexpected token tNL
    50 |    }

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:51: unmatched "|" in block argument list https://srb.help/2001
    51 |    1.times { |x, # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:54: unmatched "|" in block argument list https://srb.help/2001
    54 |    1.times { |x, # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:55: unexpected token tNL https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
    56 |    }

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:60: unmatched "|" in block argument list https://srb.help/2001
    60 |    1.times { |x, y # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:61: unexpected token tNL https://srb.help/2001
    61 |      # error: unexpected token tNL
    62 |    }

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:63: unmatched "|" in block argument list https://srb.help/2001
    63 |    1.times { |x, y # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:64: unexpected token tNL https://srb.help/2001
    64 |      # error: unexpected token tNL
    65 |      puts 'hello'

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:67: unmatched "|" in block argument list https://srb.help/2001
    67 |    1.times { |x, y # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:68: unexpected token tNL https://srb.help/2001
    68 |      # error: unexpected token tNL
    69 |      puts('hello')

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:71: unmatched "|" in block argument list https://srb.help/2001
    71 |    1.times { |x, y # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:72: unexpected token tNL https://srb.help/2001
    72 |      # error: unexpected token tNL
    73 |      x = nil

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:75: unmatched "|" in block argument list https://srb.help/2001
    75 |    1.times { |x, y # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:76: unexpected token tNL https://srb.help/2001
    76 |      # error: unexpected token tNL
    77 |      Opus::Log.info
Errors: 28
```

</details>

<details>
<summary>Prism errors (27)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_2.rb:9: expected the block parameters to end with `|` https://srb.help/2001
     9 |    1.times { | # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:12: expected the block parameters to end with `|` https://srb.help/2001
    12 |      puts 'hello'
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:15: expected the block parameters to end with `|` https://srb.help/2001
    15 |      puts('hello')
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:19: expected the block parameters to end with `|` https://srb.help/2001
    19 |    }
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:21: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    21 |      Opus::Log.info
              ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:21: expected the block parameters to end with `|` https://srb.help/2001
    21 |      Opus::Log.info
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:21: unexpected '::', ignoring it https://srb.help/2001
    21 |      Opus::Log.info
                  ^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:27: expected the block parameters to end with `|` https://srb.help/2001
    27 |    }
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:29: expected the block parameters to end with `|` https://srb.help/2001
    29 |      puts 'hello'
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:32: expected the block parameters to end with `|` https://srb.help/2001
    32 |      puts('hello')
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:35: expected the block parameters to end with `|` https://srb.help/2001
    35 |      x = nil
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:38: expected the block parameters to end with `|` https://srb.help/2001
    38 |      Opus::Log.info
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:43: expected the block parameters to end with `|` https://srb.help/2001
    43 |    1.times { |x, # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:46: expected the block parameters to end with `|` https://srb.help/2001
    46 |      puts 'hello' # error: unexpected token tSTRING
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:49: expected the block parameters to end with `|` https://srb.help/2001
    49 |      puts('hello') # error: unexpected token tNL
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:52: duplicated argument name https://srb.help/2001
    52 |      x = nil
              ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:53: expected the block parameters to end with `|` https://srb.help/2001
    53 |    }
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:55: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
              ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:55: expected the block parameters to end with `|` https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
                  ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:55: unexpected '::', ignoring it https://srb.help/2001
    55 |      Opus::Log.info # error: unexpected token tNL
                  ^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:61: expected the block parameters to end with `|` https://srb.help/2001
    61 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:64: expected the block parameters to end with `|` https://srb.help/2001
    64 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:68: expected the block parameters to end with `|` https://srb.help/2001
    68 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:72: expected the block parameters to end with `|` https://srb.help/2001
    72 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:76: expected the block parameters to end with `|` https://srb.help/2001
    76 |      # error: unexpected token tNL
        ^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:21: Unable to resolve constant `Log` https://srb.help/5002
    21 |      Opus::Log.info
                    ^^^

test/testdata/parser/error_recovery/unmatched_block_args_2.rb:55: Unable to resolve constant `Log` https://srb.help/5002
    55 |      Opus::Log.info # error: unexpected token tNL
                    ^^^
Errors: 27
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism produces spurious errors treating code elements as block parameters (e.g., Opus as constant argument, x as duplicate) and misses the simpler unmatched pipe errors that Original correctly identifies.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do ||
          x
        end
        1.times() do ||
          begin
            x
            <self>.puts("hello")
          end
        end
        1.times() do ||
          begin
            x
            <self>.puts("hello")
          end
        end
        1.times() do ||
          begin
            x
            x = nil
          end
        end
        1.times() do ||
          begin
            x
            <emptyTree>::<C Opus>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          begin
            <assignTemp>$2 = nil
            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
            x = <assignTemp>$3.[](0)
            x = <assignTemp>$3.[](1)
            <assignTemp>$2
          end
        end
        1.times() do ||
          <emptyTree>
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |x = nil|
          <emptyTree>
        end
        1.times() do |Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do |x|
          <emptyTree>
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          x = nil
        end
        1.times() do |x|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do |x, *<restargs>|
          <emptyTree>
        end
        1.times() do |x, puts|
          "hello"
        end
        1.times() do |x, puts|
          "hello"
        end
        1.times() do |x, x = nil|
          <emptyTree>
        end
        1.times() do |x, Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      begin
        1.times() do |x, y|
          <emptyTree>
        end
        1.times() do |x, y|
          <self>.puts("hello")
        end
        1.times() do |x, y|
          <self>.puts("hello")
        end
        1.times() do |x, y|
          x = nil
        end
        1.times() do |x, y|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -11,88 +11,82 @@
         1.times() do ||
           <emptyTree>
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          x = nil
+        1.times() do |x = nil|
+          <emptyTree>
         end
-        1.times() do ||
-          <emptyTree>::<C Opus>::<C Log>.info()
+        1.times() do |Opus|
+          begin
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
+          end
         end
       end
     end
 
     def test_one_arg<<todo method>>(&<blk>)
       begin
-        1.times() do ||
-          x
+        1.times() do |x|
+          <emptyTree>
         end
-        1.times() do ||
-          begin
-            x
-            <self>.puts("hello")
-          end
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          begin
-            x
-            <self>.puts("hello")
-          end
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          begin
-            x
-            x = nil
-          end
+        1.times() do |x|
+          x = nil
         end
-        1.times() do ||
-          begin
-            x
-            <emptyTree>::<C Opus>::<C Log>.info()
-          end
+        1.times() do |x|
+          <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
     end
 
     def test_one_arg_comma<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, *<restargs>|
           <emptyTree>
         end
-        1.times() do ||
-          begin
-            <assignTemp>$2 = nil
-            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
-            x = <assignTemp>$3.[](0)
-            x = <assignTemp>$3.[](1)
-            <assignTemp>$2
-          end
+        1.times() do |x, puts|
+          "hello"
         end
-        1.times() do ||
+        1.times() do |x, puts|
+          "hello"
+        end
+        1.times() do |x, x = nil|
           <emptyTree>
         end
+        1.times() do |x, Opus|
+          begin
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
+          end
+        end
       end
     end
 
     def test_two_args<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, y|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, y|
           <self>.puts("hello")
         end
-        1.times() do ||
+        1.times() do |x, y|
           <self>.puts("hello")
         end
-        1.times() do ||
+        1.times() do |x, y|
           x = nil
         end
-        1.times() do ||
+        1.times() do |x, y|
           <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
```

</details>

## unmatched_block_args_3.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module Opus::Log
  def self.info; end
end

# The commented out lines are the ones that don't parse at the time of writing
# this test. If any of them is uncommented it makes the whole file appear to
# fail to parse at all, but I didn't want to make one file for each of them in
# isolation.

class A
  def test_no_args
    1.times do | end # error: unmatched "|" in block argument list
    1.times do | puts 'hello' end # error: unmatched "|" in block argument list
    1.times do | puts('hello') end # error: unmatched "|" in block argument list
    1.times do | x = nil end # error: unmatched "|" in block argument list
    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
  end

  def test_one_arg
    1.times do |x end # error: unmatched "|" in block argument list
    1.times do |x puts 'hello' end # error: unmatched "|" in block argument list
    1.times do |x puts('hello') end # error: unmatched "|" in block argument list
    1.times do |x x = nil end # error: unmatched "|" in block argument list
    1.times do |x Opus::Log.info end # error: unmatched "|" in block argument list
  end

  def test_one_arg_comma
    1.times do |x, end # error: unmatched "|" in block argument list
    # 1.times do |x, puts 'hello' end
    1.times do |x, puts('hello') end
    #          ^ error: unmatched "|" in block argument list
    #                            ^^^ error: unexpected token "end"
    1.times do |x, x = nil end # error: unmatched "|" in block argument list
    1.times do |x, Opus::Log.info end
    #          ^ error: unmatched "|" in block argument list
    #                             ^^^ error: unexpected token "end"
  end

  def test_two_args
    1.times do |x, y end
    #          ^ error: unmatched "|" in block argument list
    #                ^^^ error: unexpected token "end"
    # 1.times do |x, y puts 'hello' end
    # 1.times do |x, y puts('hello') end
    # 1.times do |x, y x = nil end
    # 1.times do |x, y Opus::Log.info end
  end
end

```

</details>

<details>
<summary>Original errors (18)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_3.rb:14: unmatched "|" in block argument list https://srb.help/2001
    14 |    1.times do | end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:15: unmatched "|" in block argument list https://srb.help/2001
    15 |    1.times do | puts 'hello' end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:16: unmatched "|" in block argument list https://srb.help/2001
    16 |    1.times do | puts('hello') end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:17: unmatched "|" in block argument list https://srb.help/2001
    17 |    1.times do | x = nil end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:18: unmatched "|" in block argument list https://srb.help/2001
    18 |    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:22: unmatched "|" in block argument list https://srb.help/2001
    22 |    1.times do |x end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:23: unmatched "|" in block argument list https://srb.help/2001
    23 |    1.times do |x puts 'hello' end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:24: unmatched "|" in block argument list https://srb.help/2001
    24 |    1.times do |x puts('hello') end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:25: unmatched "|" in block argument list https://srb.help/2001
    25 |    1.times do |x x = nil end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:26: unmatched "|" in block argument list https://srb.help/2001
    26 |    1.times do |x Opus::Log.info end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:30: unmatched "|" in block argument list https://srb.help/2001
    30 |    1.times do |x, end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:32: unmatched "|" in block argument list https://srb.help/2001
    32 |    1.times do |x, puts('hello') end
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:32: unexpected token "end" https://srb.help/2001
    32 |    1.times do |x, puts('hello') end
                                         ^^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:35: unmatched "|" in block argument list https://srb.help/2001
    35 |    1.times do |x, x = nil end # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: unmatched "|" in block argument list https://srb.help/2001
    36 |    1.times do |x, Opus::Log.info end
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: unexpected token "end" https://srb.help/2001
    36 |    1.times do |x, Opus::Log.info end
                                          ^^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:42: unmatched "|" in block argument list https://srb.help/2001
    42 |    1.times do |x, y end
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:42: unexpected token "end" https://srb.help/2001
    42 |    1.times do |x, y end
                             ^^^
Errors: 18
```

</details>

<details>
<summary>Prism errors (22)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_3.rb:14: expected the block parameters to end with `|` https://srb.help/2001
    14 |    1.times do | end # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:15: expected the block parameters to end with `|` https://srb.help/2001
    15 |    1.times do | puts 'hello' end # error: unmatched "|" in block argument list
                             ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:16: expected the block parameters to end with `|` https://srb.help/2001
    16 |    1.times do | puts('hello') end # error: unmatched "|" in block argument list
                             ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:17: expected the block parameters to end with `|` https://srb.help/2001
    17 |    1.times do | x = nil end # error: unmatched "|" in block argument list
                                ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:18: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    18 |    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
                         ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:18: expected the block parameters to end with `|` https://srb.help/2001
    18 |    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
                             ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:18: unexpected '::', ignoring it https://srb.help/2001
    18 |    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
                             ^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:22: expected the block parameters to end with `|` https://srb.help/2001
    22 |    1.times do |x end # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:23: expected the block parameters to end with `|` https://srb.help/2001
    23 |    1.times do |x puts 'hello' end # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:24: expected the block parameters to end with `|` https://srb.help/2001
    24 |    1.times do |x puts('hello') end # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:25: expected the block parameters to end with `|` https://srb.help/2001
    25 |    1.times do |x x = nil end # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:26: expected the block parameters to end with `|` https://srb.help/2001
    26 |    1.times do |x Opus::Log.info end # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:30: expected the block parameters to end with `|` https://srb.help/2001
    30 |    1.times do |x, end # error: unmatched "|" in block argument list
                          ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:32: expected the block parameters to end with `|` https://srb.help/2001
    32 |    1.times do |x, puts('hello') end
                               ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:35: duplicated argument name https://srb.help/2001
    35 |    1.times do |x, x = nil end # error: unmatched "|" in block argument list
                           ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:35: expected the block parameters to end with `|` https://srb.help/2001
    35 |    1.times do |x, x = nil end # error: unmatched "|" in block argument list
                                  ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    36 |    1.times do |x, Opus::Log.info end
                           ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: expected the block parameters to end with `|` https://srb.help/2001
    36 |    1.times do |x, Opus::Log.info end
                               ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: unexpected '::', ignoring it https://srb.help/2001
    36 |    1.times do |x, Opus::Log.info end
                               ^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:42: expected the block parameters to end with `|` https://srb.help/2001
    42 |    1.times do |x, y end
                            ^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:18: Unable to resolve constant `Log` https://srb.help/5002
    18 |    1.times do | Opus::Log.info end # error: unmatched "|" in block argument list
                               ^^^

test/testdata/parser/error_recovery/unmatched_block_args_3.rb:36: Unable to resolve constant `Log` https://srb.help/5002
    36 |    1.times do |x, Opus::Log.info end
                                 ^^^
Errors: 22
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all the missing pipe errors with clearer messages but adds extra noise with spurious constant resolution errors and overly detailed parsing errors that don't add value.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do ||
          x
        end
        1.times() do ||
          <self>.x(<self>.puts("hello"))
        end
        1.times() do ||
          <self>.x(<self>.puts("hello"))
        end
        1.times() do ||
          <self>.x(x = nil)
        end
        1.times() do ||
          <self>.x(<emptyTree>::<C Opus>::<C Log>.info())
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          begin
            <assignTemp>$2 = nil
            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
            x = <assignTemp>$3.[](0)
            x = <assignTemp>$3.[](1)
            <assignTemp>$2
          end
        end
        1.times() do ||
          <emptyTree>
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      1.times() do ||
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |x = nil|
          <emptyTree>
        end
        1.times() do |Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do |x|
          <emptyTree>
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          x = nil
        end
        1.times() do |x|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do |x, *<restargs>|
          <emptyTree>
        end
        1.times() do |x, puts|
          "hello"
        end
        1.times() do |x, x = nil|
          <emptyTree>
        end
        1.times() do |x, Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      1.times() do |x, y|
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -11,66 +11,66 @@
         1.times() do ||
           <emptyTree>
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          x = nil
+        1.times() do |x = nil|
+          <emptyTree>
         end
-        1.times() do ||
-          <emptyTree>::<C Opus>::<C Log>.info()
+        1.times() do |Opus|
+          begin
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
+          end
         end
       end
     end
 
     def test_one_arg<<todo method>>(&<blk>)
       begin
-        1.times() do ||
-          x
+        1.times() do |x|
+          <emptyTree>
         end
-        1.times() do ||
-          <self>.x(<self>.puts("hello"))
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          <self>.x(<self>.puts("hello"))
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          <self>.x(x = nil)
+        1.times() do |x|
+          x = nil
         end
-        1.times() do ||
-          <self>.x(<emptyTree>::<C Opus>::<C Log>.info())
+        1.times() do |x|
+          <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
     end
 
     def test_one_arg_comma<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, *<restargs>|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, puts|
+          "hello"
+        end
+        1.times() do |x, x = nil|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, Opus|
           begin
-            <assignTemp>$2 = nil
-            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
-            x = <assignTemp>$3.[](0)
-            x = <assignTemp>$3.[](1)
-            <assignTemp>$2
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
           end
         end
-        1.times() do ||
-          <emptyTree>
-        end
       end
     end
 
     def test_two_args<<todo method>>(&<blk>)
-      1.times() do ||
+      1.times() do |x, y|
         <emptyTree>
       end
     end
```

</details>

## unmatched_block_args_4.rb

<details>
<summary>Input</summary>

```ruby
# typed: false

module Opus::Log
  def self.info; end
end

# The commented out lines are the ones that don't parse at the time of writing
# this test. If any of them is uncommented it makes the whole file appear to
# fail to parse at all, but I didn't want to make one file for each of them in
# isolation.

class A
  def test_no_args
    1.times { | } # error: unmatched "|" in block argument list
    1.times { | puts 'hello' } # error: unmatched "|" in block argument list
    1.times { | puts('hello') } # error: unmatched "|" in block argument list
    1.times { | x = nil } # error: unmatched "|" in block argument list
    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
  end

  def test_one_arg
    1.times { |x } # error: unmatched "|" in block argument list
    1.times { |x puts 'hello' } # error: unmatched "|" in block argument list
    1.times { |x puts('hello') } # error: unmatched "|" in block argument list
    1.times { |x x = nil } # error: unmatched "|" in block argument list
    1.times { |x Opus::Log.info } # error: unmatched "|" in block argument list
  end

  def test_one_arg_comma
    1.times { |x, } # error: unmatched "|" in block argument list
    # 1.times { |x, puts 'hello' }
    1.times { |x, puts('hello') }
    #         ^ error: unmatched "|" in block argument list
    #                           ^ error: unexpected token tRCURLY
    1.times { |x, x = nil } # error: unmatched "|" in block argument list
    1.times { |x, Opus::Log.info }
    #         ^ error: unmatched "|" in block argument list
    #                            ^ error: unexpected token tRCURLY
  end

  def test_two_args
    1.times { |x, y }
    #         ^ error: unmatched "|" in block argument list
    #               ^ error: unexpected token tRCURLY
    # 1.times { |x, y puts 'hello' }
    # 1.times { |x, y puts('hello') }
    # 1.times { |x, y x = nil }
    # 1.times { |x, y Opus::Log.info }
  end
end

```

</details>

<details>
<summary>Original errors (18)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_4.rb:14: unmatched "|" in block argument list https://srb.help/2001
    14 |    1.times { | } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:15: unmatched "|" in block argument list https://srb.help/2001
    15 |    1.times { | puts 'hello' } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:16: unmatched "|" in block argument list https://srb.help/2001
    16 |    1.times { | puts('hello') } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:17: unmatched "|" in block argument list https://srb.help/2001
    17 |    1.times { | x = nil } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:18: unmatched "|" in block argument list https://srb.help/2001
    18 |    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:22: unmatched "|" in block argument list https://srb.help/2001
    22 |    1.times { |x } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:23: unmatched "|" in block argument list https://srb.help/2001
    23 |    1.times { |x puts 'hello' } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:24: unmatched "|" in block argument list https://srb.help/2001
    24 |    1.times { |x puts('hello') } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:25: unmatched "|" in block argument list https://srb.help/2001
    25 |    1.times { |x x = nil } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:26: unmatched "|" in block argument list https://srb.help/2001
    26 |    1.times { |x Opus::Log.info } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:30: unmatched "|" in block argument list https://srb.help/2001
    30 |    1.times { |x, } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:32: unmatched "|" in block argument list https://srb.help/2001
    32 |    1.times { |x, puts('hello') }
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:32: unexpected token tRCURLY https://srb.help/2001
    32 |    1.times { |x, puts('hello') }
                                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:35: unmatched "|" in block argument list https://srb.help/2001
    35 |    1.times { |x, x = nil } # error: unmatched "|" in block argument list
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: unmatched "|" in block argument list https://srb.help/2001
    36 |    1.times { |x, Opus::Log.info }
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: unexpected token tRCURLY https://srb.help/2001
    36 |    1.times { |x, Opus::Log.info }
                                         ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:42: unmatched "|" in block argument list https://srb.help/2001
    42 |    1.times { |x, y }
                      ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:42: unexpected token tRCURLY https://srb.help/2001
    42 |    1.times { |x, y }
                            ^
Errors: 18
```

</details>

<details>
<summary>Prism errors (22)</summary>

```
test/testdata/parser/error_recovery/unmatched_block_args_4.rb:14: expected the block parameters to end with `|` https://srb.help/2001
    14 |    1.times { | } # error: unmatched "|" in block argument list
                       ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:15: expected the block parameters to end with `|` https://srb.help/2001
    15 |    1.times { | puts 'hello' } # error: unmatched "|" in block argument list
                            ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:16: expected the block parameters to end with `|` https://srb.help/2001
    16 |    1.times { | puts('hello') } # error: unmatched "|" in block argument list
                            ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:17: expected the block parameters to end with `|` https://srb.help/2001
    17 |    1.times { | x = nil } # error: unmatched "|" in block argument list
                               ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:18: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    18 |    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
                        ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:18: expected the block parameters to end with `|` https://srb.help/2001
    18 |    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
                            ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:18: unexpected '::', ignoring it https://srb.help/2001
    18 |    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
                            ^^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:22: expected the block parameters to end with `|` https://srb.help/2001
    22 |    1.times { |x } # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:23: expected the block parameters to end with `|` https://srb.help/2001
    23 |    1.times { |x puts 'hello' } # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:24: expected the block parameters to end with `|` https://srb.help/2001
    24 |    1.times { |x puts('hello') } # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:25: expected the block parameters to end with `|` https://srb.help/2001
    25 |    1.times { |x x = nil } # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:26: expected the block parameters to end with `|` https://srb.help/2001
    26 |    1.times { |x Opus::Log.info } # error: unmatched "|" in block argument list
                        ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:30: expected the block parameters to end with `|` https://srb.help/2001
    30 |    1.times { |x, } # error: unmatched "|" in block argument list
                         ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:32: expected the block parameters to end with `|` https://srb.help/2001
    32 |    1.times { |x, puts('hello') }
                              ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:35: duplicated argument name https://srb.help/2001
    35 |    1.times { |x, x = nil } # error: unmatched "|" in block argument list
                          ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:35: expected the block parameters to end with `|` https://srb.help/2001
    35 |    1.times { |x, x = nil } # error: unmatched "|" in block argument list
                                 ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: invalid formal argument; formal argument cannot be a constant https://srb.help/2001
    36 |    1.times { |x, Opus::Log.info }
                          ^^^^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: expected the block parameters to end with `|` https://srb.help/2001
    36 |    1.times { |x, Opus::Log.info }
                              ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: unexpected '::', ignoring it https://srb.help/2001
    36 |    1.times { |x, Opus::Log.info }
                              ^^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:42: expected the block parameters to end with `|` https://srb.help/2001
    42 |    1.times { |x, y }
                           ^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:18: Unable to resolve constant `Log` https://srb.help/5002
    18 |    1.times { | Opus::Log.info } # error: unmatched "|" in block argument list
                              ^^^

test/testdata/parser/error_recovery/unmatched_block_args_4.rb:36: Unable to resolve constant `Log` https://srb.help/5002
    36 |    1.times { |x, Opus::Log.info }
                                ^^^
Errors: 22
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism catches all syntax errors with clearer messages about missing closing pipes, but adds noisy secondary errors about constants and duplicate names that are artifacts of the malformed syntax, plus two spurious constant resolution errors.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          <self>.puts("hello")
        end
        1.times() do ||
          x = nil
        end
        1.times() do ||
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do ||
          x
        end
        1.times() do ||
          <self>.x(<self>.puts("hello"))
        end
        1.times() do ||
          <self>.x(<self>.puts("hello"))
        end
        1.times() do ||
          <self>.x(x = nil)
        end
        1.times() do ||
          <self>.x(<emptyTree>::<C Opus>::<C Log>.info())
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          <emptyTree>
        end
        1.times() do ||
          begin
            <assignTemp>$2 = nil
            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
            x = <assignTemp>$3.[](0)
            x = <assignTemp>$3.[](1)
            <assignTemp>$2
          end
        end
        1.times() do ||
          <emptyTree>
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      1.times() do ||
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  module <emptyTree>::<C Opus>::<C Log><<C <todo sym>>> < ()
    def self.info<<todo method>>(&<blk>)
      <emptyTree>
    end
  end

  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def test_no_args<<todo method>>(&<blk>)
      begin
        1.times() do ||
          <emptyTree>
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |puts|
          "hello"
        end
        1.times() do |x = nil|
          <emptyTree>
        end
        1.times() do |Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_one_arg<<todo method>>(&<blk>)
      begin
        1.times() do |x|
          <emptyTree>
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          <self>.puts("hello")
        end
        1.times() do |x|
          x = nil
        end
        1.times() do |x|
          <emptyTree>::<C Opus>::<C Log>.info()
        end
      end
    end

    def test_one_arg_comma<<todo method>>(&<blk>)
      begin
        1.times() do |x, *<restargs>|
          <emptyTree>
        end
        1.times() do |x, puts|
          "hello"
        end
        1.times() do |x, x = nil|
          <emptyTree>
        end
        1.times() do |x, Opus|
          begin
            <emptyTree>::<C <ErrorNode>>
            <emptyTree>::<C Log>.info()
          end
        end
      end
    end

    def test_two_args<<todo method>>(&<blk>)
      1.times() do |x, y|
        <emptyTree>
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -11,66 +11,66 @@
         1.times() do ||
           <emptyTree>
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          <self>.puts("hello")
+        1.times() do |puts|
+          "hello"
         end
-        1.times() do ||
-          x = nil
+        1.times() do |x = nil|
+          <emptyTree>
         end
-        1.times() do ||
-          <emptyTree>::<C Opus>::<C Log>.info()
+        1.times() do |Opus|
+          begin
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
+          end
         end
       end
     end
 
     def test_one_arg<<todo method>>(&<blk>)
       begin
-        1.times() do ||
-          x
+        1.times() do |x|
+          <emptyTree>
         end
-        1.times() do ||
-          <self>.x(<self>.puts("hello"))
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          <self>.x(<self>.puts("hello"))
+        1.times() do |x|
+          <self>.puts("hello")
         end
-        1.times() do ||
-          <self>.x(x = nil)
+        1.times() do |x|
+          x = nil
         end
-        1.times() do ||
-          <self>.x(<emptyTree>::<C Opus>::<C Log>.info())
+        1.times() do |x|
+          <emptyTree>::<C Opus>::<C Log>.info()
         end
       end
     end
 
     def test_one_arg_comma<<todo method>>(&<blk>)
       begin
-        1.times() do ||
+        1.times() do |x, *<restargs>|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, puts|
+          "hello"
+        end
+        1.times() do |x, x = nil|
           <emptyTree>
         end
-        1.times() do ||
+        1.times() do |x, Opus|
           begin
-            <assignTemp>$2 = nil
-            <assignTemp>$3 = ::<Magic>.<expand-splat>(<assignTemp>$2, 2, 0)
-            x = <assignTemp>$3.[](0)
-            x = <assignTemp>$3.[](1)
-            <assignTemp>$2
+            <emptyTree>::<C <ErrorNode>>
+            <emptyTree>::<C Log>.info()
           end
         end
-        1.times() do ||
-          <emptyTree>
-        end
       end
     end
 
     def test_two_args<<todo method>>(&<blk>)
-      1.times() do ||
+      1.times() do |x, y|
         <emptyTree>
       end
     end
```

</details>

## unterminated_array.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

class A
  def f
    [1,
  #   ^ error: unexpected token ","
  # ^ error: unterminated "["
    puts "hi"
  end
  def g
    [
  # ^ error: unterminated "["
  end
  def h
    puts "ho"
  end
end

class B
  def q
    puts "ho"
    [
  # ^ error: unterminated "["
  end
  def r
    puts "hi"
  end
  def s
    puts ([] + [)
             # ^ error: unterminated "["
  end
  def t
    puts "hi"
  end
end

```

</details>

<details>
<summary>Original errors (5)</summary>

```
test/testdata/parser/error_recovery/unterminated_array.rb:5: unexpected token "," https://srb.help/2001
     5 |    [1,
              ^

test/testdata/parser/error_recovery/unterminated_array.rb:5: unterminated "[" https://srb.help/2001
     5 |    [1,
            ^

test/testdata/parser/error_recovery/unterminated_array.rb:11: unterminated "[" https://srb.help/2001
    11 |    [
            ^

test/testdata/parser/error_recovery/unterminated_array.rb:22: unterminated "[" https://srb.help/2001
    22 |    [
            ^

test/testdata/parser/error_recovery/unterminated_array.rb:29: unterminated "[" https://srb.help/2001
    29 |    puts ([] + [)
                       ^
Errors: 5
```

</details>

<details>
<summary>Prism errors (12)</summary>

```
test/testdata/parser/error_recovery/unterminated_array.rb:8: unexpected string literal; expected a `,` separator for the array elements https://srb.help/2001
     8 |    puts "hi"
                ^

test/testdata/parser/error_recovery/unterminated_array.rb:9: unexpected 'end'; expected a `,` separator for the array elements https://srb.help/2001
     9 |  end
        ^

test/testdata/parser/error_recovery/unterminated_array.rb:9: expected an expression for the array element https://srb.help/2001
     9 |  end
        ^

test/testdata/parser/error_recovery/unterminated_array.rb:9: unexpected 'end'; expected a `]` to close the array https://srb.help/2001
     9 |  end
          ^^^

test/testdata/parser/error_recovery/unterminated_array.rb:11: expected an expression for the array element https://srb.help/2001
    11 |    [
            ^

test/testdata/parser/error_recovery/unterminated_array.rb:13: unexpected 'end'; expected a `]` to close the array https://srb.help/2001
    13 |  end
          ^^^

test/testdata/parser/error_recovery/unterminated_array.rb:22: expected an expression for the array element https://srb.help/2001
    22 |    [
            ^

test/testdata/parser/error_recovery/unterminated_array.rb:24: unexpected 'end'; expected a `]` to close the array https://srb.help/2001
    24 |  end
          ^^^

test/testdata/parser/error_recovery/unterminated_array.rb:29: expected an expression for the array element https://srb.help/2001
    29 |    puts ([] + [)
                       ^

test/testdata/parser/error_recovery/unterminated_array.rb:29: unexpected ')'; expected a `]` to close the array https://srb.help/2001
    29 |    puts ([] + [)
                        ^

test/testdata/parser/error_recovery/unterminated_array.rb:29: expected an `end` to close the `def` statement https://srb.help/2001
    29 |    puts ([] + [)
                         ^

test/testdata/parser/error_recovery/unterminated_array.rb:29: expected an `end` to close the `class` statement https://srb.help/2001
    29 |    puts ([] + [)
                         ^
Errors: 12
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism produces 12 errors versus Original's 5, with overly verbose cascading errors and misses the simple unterminated bracket diagnostic that Original provides at the actual problem location.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def f<<todo method>>(&<blk>)
      [<emptyTree>::<C <ErrorNode>>]
    end

    def g<<todo method>>(&<blk>)
      []
    end

    def h<<todo method>>(&<blk>)
      <self>.puts("ho")
    end
  end

  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def q<<todo method>>(&<blk>)
      begin
        <self>.puts("ho")
        []
      end
    end

    def r<<todo method>>(&<blk>)
      <self>.puts("hi")
    end

    def s<<todo method>>(&<blk>)
      <self>.puts([].+([]))
    end

    def t<<todo method>>(&<blk>)
      <self>.puts("hi")
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def f<<todo method>>(&<blk>)
      [1, <self>.puts(), "hi", <emptyTree>::<C <ErrorNode>>]
    end

    def g<<todo method>>(&<blk>)
      [<emptyTree>::<C <ErrorNode>>]
    end

    def h<<todo method>>(&<blk>)
      <self>.puts("ho")
    end
  end

  class <emptyTree>::<C B><<C <todo sym>>> < (::<todo sym>)
    def q<<todo method>>(&<blk>)
      begin
        <self>.puts("ho")
        [<emptyTree>::<C <ErrorNode>>]
      end
    end

    def r<<todo method>>(&<blk>)
      <self>.puts("hi")
    end

    def s<<todo method>>(&<blk>)
      <self>.puts([].+([<emptyTree>::<C <ErrorNode>>]))
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,11 +1,11 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
     def f<<todo method>>(&<blk>)
-      [<emptyTree>::<C <ErrorNode>>]
+      [1, <self>.puts(), "hi", <emptyTree>::<C <ErrorNode>>]
     end
 
     def g<<todo method>>(&<blk>)
-      []
+      [<emptyTree>::<C <ErrorNode>>]
     end
 
     def h<<todo method>>(&<blk>)
@@ -17,7 +17,7 @@
     def q<<todo method>>(&<blk>)
       begin
         <self>.puts("ho")
-        []
+        [<emptyTree>::<C <ErrorNode>>]
       end
     end
 
@@ -26,11 +26,7 @@
     end
 
     def s<<todo method>>(&<blk>)
-      <self>.puts([].+([]))
+      <self>.puts([].+([<emptyTree>::<C <ErrorNode>>]))
     end
-
-    def t<<todo method>>(&<blk>)
-      <self>.puts("hi")
-    end
   end
 end
```

</details>

## unterminated_array_bad_1.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

# We'd like a better parse for this case than we currently are getting. Specifically it would be nice if it didn't drop
# the 1 from the array, nor drop the sig + method def. If you can get closer to the desired behavior, please feel free
# to update the .exp file for this test.

class A
  X = [1,
    # ^ error: unterminated "["
  sig {void}
  def bar
# ^^^ error: unexpected token "def"
  end
end # error: unexpected token "end"

```

</details>

<details>
<summary>Original errors (3)</summary>

```
test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:11: unexpected token "def" https://srb.help/2001
    11 |  def bar
          ^^^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:8: unterminated "[" https://srb.help/2001
     8 |  X = [1,
              ^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:14: unexpected token "end" https://srb.help/2001
    14 |end # error: unexpected token "end"
        ^^^
Errors: 3
```

</details>

<details>
<summary>Prism errors (6) | Autocorrects: 1</summary>

```
test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:11: unexpected 'def'; expected a `,` separator for the array elements https://srb.help/2001
    11 |  def bar
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:14: unexpected 'end'; expected a `,` separator for the array elements https://srb.help/2001
    14 |end # error: unexpected token "end"
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:14: expected an expression for the array element https://srb.help/2001
    14 |end # error: unexpected token "end"
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:14: unexpected 'end'; expected a `]` to close the array https://srb.help/2001
    14 |end # error: unexpected token "end"
        ^^^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:10: Method `sig` does not exist on `T.class_of(A)` https://srb.help/7003
    10 |  sig {void}
          ^^^
  Got `T.class_of(A)` originating from:
    test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:7:
     7 |class A
        ^
  Autocorrect: Use `-a` to autocorrect
    test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:8: Insert `extend T::Sig`
     8 |  X = [1,
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:10: Method `void` does not exist on `T.class_of(A)` https://srb.help/7003
    10 |  sig {void}
               ^^^^
  Got `T.class_of(A)` originating from:
    test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:7:
     7 |class A
        ^
  Did you mean `load`? Use `-a` to autocorrect
    test/testdata/parser/error_recovery/unterminated_array_bad_1.rb:10: Replace with `load`
    10 |  sig {void}
               ^^^^
    https://github.com/sorbet/sorbet/tree/master/rbi/core/kernel.rbi#L1664: Defined here
    1664 |  def load(filename, arg0=T.unsafe(nil)); end
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Errors: 6
```

</details>

<details>
<summary>LLM quality assessment 🔴</summary>

Prism fails to recognize the unterminated array and instead treats sig/def/end as array elements, missing the core parse error while generating spurious type-checking errors.

</details>

<details>
<summary>Prism autocorrect diff</summary>

```diff
--- Original
+++ Autocorrected
@@ -5,9 +5,10 @@
 # to update the .exp file for this test.
 
 class A
+  extend T::Sig
   X = [1,
     # ^ error: unterminated "["
-  sig {void}
+  sig {load}
   def bar
 # ^^^ error: unexpected token "def"
   end
```

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <emptyTree>::<C X> = [<emptyTree>::<C <ErrorNode>>]
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    <emptyTree>::<C X> = [1, <self>.sig() do ||
        <self>.void()
      end, def bar<<todo method>>(&<blk>)
        <emptyTree>
      end, <emptyTree>::<C <ErrorNode>>]
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🔴</summary>

```diff
--- Original
+++ Prism
@@ -1,5 +1,9 @@
 class <emptyTree><<C <root>>> < (::<todo sym>)
   class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
-    <emptyTree>::<C X> = [<emptyTree>::<C <ErrorNode>>]
+    <emptyTree>::<C X> = [1, <self>.sig() do ||
+        <self>.void()
+      end, def bar<<todo method>>(&<blk>)
+        <emptyTree>
+      end, <emptyTree>::<C <ErrorNode>>]
   end
 end
```

</details>

## unterminated_array_bad_2.rb

<details>
<summary>Input</summary>

```ruby
# typed: true

# We'd like a better parse for this case than we currently are getting. Specifically it would be nice if it didn't drop
# the 1 from the array, nor drop the "puts 'after'". If you can get closer to the desired behavior, please feel free to
# update the .exp file for this test.

class A
  def bar
    puts 'before'
    x = [1,
      # ^ error: unterminated "["
        # ^ error: unexpected token ","
    puts 'after'
  end
end

```

</details>

<details>
<summary>Original errors (2)</summary>

```
test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:10: unexpected token "," https://srb.help/2001
    10 |    x = [1,
                  ^

test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:10: unterminated "[" https://srb.help/2001
    10 |    x = [1,
                ^
Errors: 2
```

</details>

<details>
<summary>Prism errors (4)</summary>

```
test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:13: unexpected string literal; expected a `,` separator for the array elements https://srb.help/2001
    13 |    puts 'after'
                ^

test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:14: unexpected 'end'; expected a `,` separator for the array elements https://srb.help/2001
    14 |  end
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:14: expected an expression for the array element https://srb.help/2001
    14 |  end
        ^

test/testdata/parser/error_recovery/unterminated_array_bad_2.rb:14: unexpected 'end'; expected a `]` to close the array https://srb.help/2001
    14 |  end
          ^^^
Errors: 4
```

</details>

<details>
<summary>LLM quality assessment 🟡</summary>

Prism provides more detailed error messages and better recovery by attempting to parse subsequent code, but produces 4 errors instead of 2 and points to later lines rather than the actual problem location at line 10.

</details>

<details>
<summary>Original desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def bar<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        x = [<emptyTree>::<C <ErrorNode>>]
      end
    end
  end
end
```

</details>

<details>
<summary>Prism desugar tree</summary>

```
class <emptyTree><<C <root>>> < (::<todo sym>)
  class <emptyTree>::<C A><<C <todo sym>>> < (::<todo sym>)
    def bar<<todo method>>(&<blk>)
      begin
        <self>.puts("before")
        x = [1, <self>.puts(), "after", <emptyTree>::<C <ErrorNode>>]
      end
    end
  end
end
```

</details>

<details>
<summary>Desugar tree diff 🟡</summary>

```diff
--- Original
+++ Prism
@@ -3,7 +3,7 @@
     def bar<<todo method>>(&<blk>)
       begin
         <self>.puts("before")
-        x = [<emptyTree>::<C <ErrorNode>>]
+        x = [1, <self>.puts(), "after", <emptyTree>::<C <ErrorNode>>]
       end
     end
   end
```

</details>

