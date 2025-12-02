
// gs.freshNameUnique(UniqueNameKind::Singleton, this->name, 1)

Failing tests:
//test:test_PrismPosTests/testdata/desugar/forward_args
//test:test_PrismPosTests/testdata/infer/arg_matching
//test:test_PrismPosTests/testdata/infer/autocorrect_no_block
//test:test_PrismPosTests/testdata/infer/constructors
//test:test_PrismPosTests/testdata/lsp/completion/empty_parens
//test:test_PrismPosTests/testdata/lsp/completion/method_kwargs_extra
//test:test_PrismPosTests/testdata/parser/bad_argument_crash
//test:test_PrismPosTests/testdata/parser/compare_overload_parse_error
//test:test_PrismPosTests/testdata/parser/end_data
//test:test_PrismPosTests/testdata/parser/kwargs_missing_comma_unsupported
//test:test_PrismPosTests/testdata/rbi/pathname
//test:test_PrismPosTests/testdata/rbs/assertions_heredoc_modified
//test:test_PrismPosTests/testdata/rbs/signatures_defs
//test:test_PrismPosTests/testdata/rbs/signatures_defs_multiline
//test:test_PrismPosTests/testdata/resolver/forward_args
//test:test_PrismPosTests/testdata/rewriter/cast_block
//test:test_PrismPosTests/testdata/rewriter/initializer_no_arg_with_local





// When I actually fix the comparisons:
// Executed 2082 out of 2082 tests: 1805 tests pass and 277 fail locally.

// After fixing rescues:
// Executed 2083 out of 2083 tests: 1909 tests pass and 174 fail locally.

// After fixing a bunch more shit:
// Executed 2083 out of 2083 tests: 1976 tests pass and 107 fail locally.

// After fixing Regexp desugared content loc:
// Executed 2083 out of 2083 tests: 1996 tests pass and 87 fail locally
