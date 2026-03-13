# Comparison: lonely_def_NN.rb Desugar Tree (Standard vs Prism)

This document compares all 13 pairs of desugaring outputs for the `lonely_def_*.rb` test cases, contrasting the original parser output (`.exp`) against the Prism-based parser output (`.prism.exp`).

## Key Finding: No `$` Variable Numbering Differences

None of the test files use `$` variables, so there are no variable numbering differences to report. The differences observed are structural in nature.

---

## Case-by-Case Comparison

### Test 01: Class with nested class, lonely def without name

**Source:** `class A { sig {void} def # broken def, followed by sig {void} def example; end }`

**Expected (.exp):**
- Class nesting: `<root>` > `A`
- Two separate method definitions: unnamed (missing name), then `example`
- Both have `<emptyTree>` bodies (no code)

**Prism (.prism.exp):**
- Parses the first `sig` as being on a method named `sig` (misinterpretation)
- Wraps remaining code in a `begin...end` block within that `sig` method
- Creates `ErrorNode` to mark error location
- Structure: `def sig { ... }` containing nested `def example` and error node

**Key difference:** Error recovery strategy differs. Standard parser treats isolated `sig` and `def` independently; Prism associates `sig` with a malformed method definition and nests the error context.

---

### Test 02: Class A with extend, lonely def at top level (no class wrapper)

**Source:** `class A { extend T::Sig, sig {params(x: Integer).void}, def # broken }`

**Expected (.exp):**
- Single `def` without name, with `<emptyTree>` body

**Prism (.prism.exp):**
- Same structure, but method name becomes `end` instead of `<method-def-name-missing>`
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Prism interprets the trailing `end` keyword as the method name and marks the body as an error; standard parser recognizes it's missing a name.

---

### Test 03: Top-level lonely def (file scope)

**Source:** `def # broken def` (at file level)

**Expected (.exp):**
- Single `def` without name, with `<emptyTree>` body

**Prism (.prism.exp):**
- Method definition has a line break: `def` on one line, `<<todo method>>(&<blk>)` on next
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Prism includes formatting differences (line breaks) in output; also marks body as error.

---

### Test 04: Standalone def with end keyword (at file level)

**Source:** `def # broken, followed by end`

**Expected (.exp):**
- Single `def` without name, with `<emptyTree>` body

**Prism (.prism.exp):**
- Method name becomes `end` instead of `<method-def-name-missing>`
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Prism interprets trailing `end` as method name; standard parser doesn't.

---

### Test 05: Class with nested class, lonely def inside

**Source:** `class A { class A { def # broken } }`

**Expected (.exp):**
- Nested class structure preserved
- Method without name, with `<emptyTree>` body

**Prism (.prism.exp):**
- Same structure but method name becomes `end` instead of `<method-def-name-missing>`
- Body is `<emptyTree>` (same as standard)
- Extra `ErrorNode` at class level

**Key difference:** Method name interpretation differs; Prism adds error marker at class scope.

---

### Test 06: Class with sig/def/self pattern

**Source:** `class A { sig {void}, def self # broken, sig {void}, def example; end }`

**Expected (.exp):**
- Two separate method defs: `def self` (missing name), then `def example`
- Both have `<emptyTree>` bodies
- No nesting

**Prism (.prism.exp):**
- First `def self` wraps subsequent code in `begin...end`
- Contains nested `sig {void}` and `def example`
- Error node at class level
- More aggressive error recovery/nesting

**Key difference:** Prism nests remaining code inside the broken `def self` method; standard parser treats them independently.

---

### Test 07: Class with sig/def self./sig/def example pattern

**Source:** `class A { sig {void}, def self. # broken, sig {void}, def example; end }`

**Expected (.exp):**
- Nested method defs: `def self.<missing>` and separate `def example`
- Both have `<emptyTree>` bodies

**Prism (.prism.exp):**
- First `def self.sig` (interprets `sig` as method name) wraps remaining code in `begin...end`
- Contains nested `sig` call and `def example`
- Error node at class level
- Similar nesting/recovery pattern as Test 06

**Key difference:** Prism interprets the method name differently (`sig` instead of missing) and nests remaining code.

---

### Test 08: Class with extend, T::Sig, sig/def self pattern

**Source:** `class A { extend T::Sig, sig {params(x: Integer).void}, def self # broken }`

**Expected (.exp):**
- Single `def self` without proper name, with `<emptyTree>` body

**Prism (.prism.exp):**
- Same structure but body is `<emptyTree>` (differs from Test 06 which had nesting)
- Extra `ErrorNode` marker at class scope

**Key difference:** Less aggressive nesting; similar to Test 05's pattern.

---

### Test 09: Class with extend, sig/def self. pattern

**Source:** `class A { extend T::Sig, sig {params(x: Integer).void}, def self. # broken }`

**Expected (.exp):**
- Single `def self.<missing>`, with `<emptyTree>` body

**Prism (.prism.exp):**
- Method name becomes `end` instead of `<method-def-name-missing>`
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Prism misinterprets `end` as method name; standard parser recognizes missing name.

---

### Test 10: Standalone def self (at file level)

**Source:** `def self # broken` (at file level)

**Expected (.exp):**
- Single `def self` with `<emptyTree>` body

**Prism (.prism.exp):**
- Same structure but body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Body interpretation differs (error vs. empty).

---

### Test 11: Standalone def self. (at file level)

**Source:** `def self. # broken` (at file level)

**Expected (.exp):**
- Single `def self.<missing>` with `<emptyTree>` body

**Prism (.prism.exp):**
- Output spans two lines for method signature: `def self.` on line 2, then `<<todo method>>(&<blk>)` on line 3
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Formatting differs (line breaks); body interpretation differs.

---

### Test 12: Standalone def self. with end

**Source:** `def self. # broken, followed by end`

**Expected (.exp):**
- Single `def self.<missing>` with `<emptyTree>` body

**Prism (.prism.exp):**
- Method name becomes `end` instead of `<method-def-name-missing>`
- Body is `ErrorNode` instead of `<emptyTree>`

**Key difference:** Prism interprets `end` as method name.

---

### Test 13: Class with def self. (inside class body)

**Source:** `class A { def self. # broken (no end), followed by end of class }`

**Expected (.exp):**
- Single `def self.<missing>` with `<emptyTree>` body

**Prism (.prism.exp):**
- Method name becomes `end` instead of `<method-def-name-missing>`
- Body is `<emptyTree>` (same as standard)
- Extra `ErrorNode` at class scope

**Key difference:** Method name interpretation differs; Prism adds error marker.

---

## Summary of Patterns

### Bodies: `<emptyTree>` vs `ErrorNode`

Most tests show Prism replacing `<emptyTree>` with `ErrorNode` in method bodies:
- Standard parser: marks incomplete methods with `<emptyTree>` (placeholder for missing body)
- Prism parser: explicitly marks them as `ErrorNode` (error recovery construct)

### Method Names: `<method-def-name-missing>` vs Keywords

Several tests show Prism interpreting trailing keywords as method names:
- Test 02, 04: `end` becomes method name instead of marked as missing
- Test 07: `sig` becomes method name instead of marked as missing
- Test 09, 12, 13: Same `end` → method name pattern

### Code Nesting

Tests 01 and 06 show aggressive nesting in Prism:
- Following code wrapped in `begin...end` within the broken method
- Allows Prism to continue parsing subsequent statements as nested content
- Standard parser treats them independently

### Formatting

Test 03 and 11 show line break handling:
- Prism preserves line breaks in method signature output
- Standard parser normalizes to single line

### Error Markers

Many Prism outputs include explicit `::ErrorNode` at scope level:
- Marks recovery point in error handling
- Standard parser omits these markers
