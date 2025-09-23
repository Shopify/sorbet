### Findings for CommentsAssociatorPrism

- **Prism node config location**: `/Users/kaanozkan/src/github.com/ruby/prism/config.yml` under `nodes:`. Node fields use `name`, `type`, optional `kind`, and `comment`. Generators that consume it: `templates/template.rb`, `rust/ruby-prism/build.rs`.

- **Dispatch and location handling**:
  - **Whitequark**: `typecase` over `parser::Node` subclasses; uses `node->loc` directly.
  - **Prism**: `switch` over `pm_node_t::type`; uses `translateLocation(pm_location_t)` on `node->location`.

- **Assertion/signature association helpers**: `consumeCommentsBetweenLines`, `consumeCommentsUntilLine`, `associateSignatureCommentsToNode`, `associateAssertionCommentsToNode` adapted to Prism locs with equivalent behavior.

- **Heredoc target line**: Same regex-based strategy. Prism checks `PM_STRING_NODE` and `PM_INTERPOLATED_STRING_NODE`; also drills into arrays/calls to find target line.

- **Begin/Block differences**:
  - **Begin**: Whitequark splits `Begin` vs `Kwbegin`. Prism uses a single `PM_BEGIN_NODE`; explicit begin is detected via `begin_keyword_loc`. Implicit-begin workaround mirrors legacy behavior by comparing first statement end to node end.
  - **Block**: Whitequark `Block` contains the call; Prism separates `PM_BLOCK_NODE` and `PM_CALL_NODE`. Current Prism implementation visits only the block body; the call handling is planned in the `PM_CALL_NODE` case.

- **Body wrapping and standalone placeholders**:
  - Whitequark inserts standalone placeholders (bind/type alias) between statements and wraps with a new `Begin` when needed.
  - Prism `maybeInsertStandalonePlaceholders` is currently stubbed; `walkBody` contains a TODO to construct a proper `PM_BEGIN_NODE` wrapper when before/after nodes are present.

- **Control structures parity**:
  - **If**: Associates on single-line and multi-line edges; Prism walks `statements` and `subsequent` via `walkBody` similar to then/else.
  - **Rescue/Ensure**: Structural differences require bespoke traversal in Prism. Ensure currently visits only `statements`; Prism’s ensure-part handling needs verification/addition.
  - **Loops (while/until)**: Whitequark has separate post forms; Prism uses `PM_LOOP_FLAGS_BEGIN_MODIFIER` to detect modifier form and adjusts traversal accordingly.

- **Calls/sends**:
  - Whitequark handles visibility sends, attr accessors, setters, and `[]=` specially (including reverse arg walk for assignments).
  - Prism `PM_CALL_NODE` handler is commented out; needs implementation for feature parity (visibility/attr accessor detection, setter/`[]=` treatment, args handling).

- **Hashes/kwargs**:
  - Whitequark avoids associating assertions for kwargs hashes.
  - Prism currently always associates on `PM_HASH_NODE`; kwargs detection remains TODO.

- **Pattern matching**:
  - Whitequark `InPattern` includes a `guard`.
  - Prism `PM_IN_NODE` does not expose a guard field; traversal visits `pattern` and `statements` only.

### Proposed follow-ups for Prism parity

- Implement `PM_CALL_NODE` handling (visibility/attr_accessor/setter/`[]=` cases, argument order).
- Implement `maybeInsertStandalonePlaceholders` and construct wrapper `PM_BEGIN_NODE` in `walkBody` when needed.
- Extend Ensure to visit both body and ensure-part fields as applicable in Prism.
- Distinguish kwargs for `PM_HASH_NODE` before associating assertions.
- Ensure `PM_BLOCK_NODE` + related `PM_CALL_NODE` are both visited so associations are not missed.


