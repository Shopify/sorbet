# typed: false

def with_named_block_paramer(&block)
  yield
  yield 123
  yield "multiple", "values"
end


def with_anonymous_block_paramer(&)
  yield
  yield 123
  yield "multiple", "values"
end

# Invalid yields outside of a method
yield
yield 123
yield "multiple", "values"
