load("//tools:clang.bzl", "clang_tool")  # todo: this should be decoupled and use the library toolchain, not the compiler one
load(":pipeline_test.bzl", "pipeline_tests")

clang_tool("llvm-symbolizer")

cc_binary(
    name = "pipeline_test_runner",
    testonly = 1,
    srcs = [
        "pipeline_test_runner.cc",
    ],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast/desugar",
        "//ast/treemap",
        "//cfg",
        "//cfg/builder",
        "//class_flatten",
        "//core",
        "//definition_validator",
        "//infer",
        "//local_vars",
        "//main/autogen",
        "//main/lsp",
        "//main/minimize",
        "//main/pipeline",
        "//main/pipeline/semantic_extension:none",
        "//namer",
        "//packager",
        "//payload",
        "//resolver",
        "//rewriter",
        "//test/helpers",
        "@cxxopts",
        "@doctest//doctest",
        "@doctest//doctest:custom_main",
    ],
)

cc_binary(
    name = "lsp_test_runner",
    testonly = 1,
    srcs = [
        "lsp_test_runner.cc",
    ],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//common",
        "//main/lsp",
        "//payload",
        "//test/helpers",
        "@cxxopts",
        "@doctest//doctest",
        "@doctest//doctest:custom_main",
    ],
)

cc_binary(
    name = "parser_test_runner",
    testonly = 1,
    srcs = [
        "parser_test_runner.cc",
    ],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast/desugar",
        "//ast/treemap",
        "//cfg",
        "//cfg/builder",
        "//class_flatten",
        "//core",
        "//definition_validator",
        "//infer",
        "//local_vars",
        "//main/autogen",
        "//main/pipeline/semantic_extension:none",
        "//namer",
        "//payload",
        "//resolver",
        "//rewriter",
        "//test/helpers",
        "@cxxopts",
        "@doctest//doctest",
        "@doctest//doctest:custom_main",
    ],
)

cc_binary(
    name = "print_document_symbols",
    testonly = 1,
    srcs = [
        "print_document_symbols.cc",
    ],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//main/lsp",
        "//payload",
    ],
)

sh_test(
    name = "print_document_symbols_builds",
    srcs = ["print_document_symbols_builds.sh"],
    data = [
        ":print_document_symbols",
    ],
)

cc_test(
    name = "hello-test",
    size = "small",
    srcs = ["hello-test.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast",
        "//ast/treemap",
        "//common",
        "//core",
        "//core/serialize",
        "//parser",
        "//payload",
        "@cxxopts",
        "@doctest//doctest",
        "@doctest//doctest:main",
    ],
)

cc_test(
    name = "pkg-autocorrects-test",
    size = "small",
    srcs = ["pkg_autocorrects_test.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast",
        "//ast/desugar",
        "//common",
        "//common/concurrency",
        "//core",
        "//local_vars",
        "//packager",
        "//parser",
        "//test/helpers",
        "@doctest//doctest",
        "@doctest//doctest:main",
    ],
)

cc_test(
    name = "pkg-condensation-test",
    size = "small",
    srcs = ["pkg_condensation_test.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast",
        "//ast/desugar",
        "//common",
        "//common/concurrency",
        "//core",
        "//local_vars",
        "//packager",
        "//parser",
        "//test/helpers",
        "@doctest//doctest",
        "@doctest//doctest:main",
    ],
)

cc_test(
    name = "error-check-test",
    size = "small",
    srcs = ["error-check-test.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast",
        "//ast/desugar",
        "//common",
        "//core",
        "//parser",
        "@doctest//doctest",
        "@doctest//doctest:main",
    ],
)

cc_test(
    name = "cancelable-indexer-test",
    size = "small",
    srcs = ["cancelable-indexer-test.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = [
        "//ast",
        "//common",
        "//core",
        "//main/options",
        "//main/pipeline",
        "//payload",
        "@doctest//doctest",
        "@doctest//doctest:main",
    ],
)

# Passes with --config=dbg but I'm not smart enough to figure out how to make it
# only run when in that config
# sh_test(
#     name = "backtrace-test",
#     size = "small",
#     srcs = ["backtrace-test.sh"],
#     data = ["backtrace-test-raise"],
# )

cc_binary(
    name = "backtrace-test-raise",
    srcs = ["backtrace-test-raise.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
)

cc_binary(
    name = "backtrace-test-error",
    srcs = ["backtrace-test-error.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//tools:__pkg__"],
    deps = ["//common"],
)

pipeline_tests(
    "test_corpus",
    glob(
        [
            "testdata/**/*.rb",
            "testdata/**/*.exp",
        ],
        exclude = [
            # Specific test that is modified for Prism, contains minor differences in the exp file
            "testdata/rbs/assertions_heredoc_modified.rb",
            "testdata/rbs/signatures_types_modified.rb",
        ],
    ),
    "PosTests",
)

pipeline_tests(
    "test_corpus_prism",
    glob(
        [
            # Replace [parser] with other phases to test Prism at that level
            # Phases: https://github.com/Shopify/sorbet/blob/prism/docs/internals.md#phases
            "testdata/**/*.rb",
            "testdata/**/*.exp",
        ],
        exclude = [
            # Tests having to do with tree differences in invalid Ruby code; will address later
            "testdata/parser/error_recovery/**",
            "testdata/parser/block_forwarding_permutations.rb",
            "testdata/parser/crash_block_pass_suggestion.rb",
            "testdata/parser/invalid_fatal.rb",
            "testdata/parser/invalid_syntax_error.rb",
            "testdata/parser/kwargs_missing_comma.rb",
            "testdata/parser/kwnil_errors.rb",  # See https://github.com/Shopify/sorbet/issues/393
            "testdata/parser/misc.rb",
            "testdata/parser/offset0.rb",
            "testdata/parser/ruby_25.rb",
            "testdata/parser/kwargs.rb",
            "testdata/lsp/completion/bad_arguments.rb",
            "testdata/lsp/completion/bad_list_elems.rb",
            "testdata/lsp/completion/case_1.rb",
            "testdata/lsp/completion/case_2.rb",
            "testdata/lsp/completion/eof.rb",
            "testdata/lsp/completion/missing_const_name.rb",
            "testdata/lsp/completion/missing_fun.rb",
            "testdata/lsp/completion/self_receiver.rb",
            "testdata/lsp/duplicate_kwarg.rb",
            "testdata/namer/fuzz_repeated_kwarg.rb",
            "testdata/deviations/keyword_method_names.rb",
            "testdata/rewriter/minitest_empty_test_each.rb",
            "testdata/infer/crash_after_parse_errors.rb",
            "testdata/parser/invalid_def_block_param_after_forward.rb",
            "testdata/parser/invalid_def_forward_after_block_param.rb",

            # Sorbets' parser incorrectly accepts invalid syntax
            "testdata/desugar/pattern_matching_hash.rb",  # See https://github.com/Shopify/sorbet/issues/362

            # Minor differences in the parse tree lead to different exp files
            "testdata/rbs/assertions_heredoc.rb",

            # Whitequark and Prism parsers disagree on inclusion of parentheses in the location.
            # This is surfaced with PM_AND_NODE and PM_OR_NODE desugaring a right hand side enclosed in parentheses.
            # See comment in PM_PARENTHESES_NODE in Translator.cc.
            #   "Override the begin node location to be the parentheses location instead of the statements location"
            # However, undoing that override leads to issues with nested parentheses handling.
            "testdata/infer/control_flow/complex_implication_1.rb",

            # Prism doesn't have enough context to reject f(&b, 1), and attempts to treat it equivalently to f(1, &b)
            "testdata/desugar/bad_block_pass.rb",

            # Additional failing tests for Prism parser / desugarer
            "testdata/compiler/block_iseq_names.rb",
            "testdata/compiler/block_return_unsafe.rb",
            "testdata/compiler/destructuring_assignment.rb",
            "testdata/compiler/exceptions/lambda_return_over_exception_frames_compiled_compiled.rb",
            "testdata/compiler/exceptions/lambda_return_over_exception_frames_interpreted_compiled.rb",
            "testdata/compiler/exceptions/raise_across_interop__*.rb",
            "testdata/compiler/exceptions/rescue_in_block.rb",
            "testdata/compiler/intrinsics/array_compact.rb",
            "testdata/compiler/intrinsics/array_compact_bang.rb",
            "testdata/compiler/intrinsics/array_find_blk.rb",
            "testdata/compiler/intrinsics/array_find_no_blk.rb",
            "testdata/compiler/intrinsics/array_reject_bang.rb",
            "testdata/compiler/intrinsics/hash_merge.rb",
            "testdata/compiler/intrinsics/hash_merge_bang.rb",
            "testdata/compiler/lambda_return_compiled_compiled.rb",
            "testdata/compiler/lambda_return_compiled_interpreted.rb",
            "testdata/compiler/lambda_return_interpreted_compiled.rb",
            "testdata/compiler/lambda_return_over_exception_frames_compiled_interpreted.rb",
            "testdata/compiler/prop_foreign.rb",
            "testdata/compiler/self_lambda_constant.rb",
            "testdata/core/fuzz_unparsable.rb",
            "testdata/desugar/block_given.rb",
            "testdata/desugar/blockpass.rb",
            "testdata/desugar/csend.rb",
            "testdata/desugar/destructure.rb",
            "testdata/desugar/fuzz_block_pass.rb",
            "testdata/desugar/fuzz_break_do_end.rb",
            "testdata/desugar/lambda.rb",
            "testdata/desugar/multi_assign.rb",
            "testdata/desugar/numbered_parameters.rb",
            "testdata/desugar/pattern_matching_const.rb",
            "testdata/desugar/pattern_matching_pins.rb",
            "testdata/desugar/splat.rb",
            "testdata/infer/autocorrect_no_block.rb",
            "testdata/infer/blockpass.rb",
            "testdata/infer/bound_proc.rb",
            "testdata/infer/call_with_block_missing.rb",
            "testdata/infer/class_new_block.rb",
            "testdata/infer/fuzz_uninitialized_vars.rb",
            "testdata/infer/generics/type_var_positive_polarity_lower_bound.rb",
            "testdata/infer/generics/unwrap_type.rb",
            "testdata/infer/generics/validator.rb",
            "testdata/infer/infer_lambda.rb",
            "testdata/infer/invariant_type_parameter.rb",
            "testdata/infer/lambda_block_return.rb",
            "testdata/infer/missing_method_name.rb",
            "testdata/infer/multi_assign.rb",
            "testdata/infer/no_valid_instantiation_result.rb",
            "testdata/infer/numbered_parameters.rb",
            "testdata/infer/or_and_conditional_branch.rb",
            "testdata/infer/sealed_list_module.rb",
            "testdata/infer/splat.rb",
            "testdata/infer/strong_splat.rb",
            "testdata/infer/t_let_lambda.rb",
            "testdata/infer/t_let_proc_type_parameter.rb",
            "testdata/infer/t_proc_metatype.rb",
            "testdata/infer/untyped.rb",
            "testdata/infer/void_proc.rb",
            "testdata/lsp/code_actions/convert_to_singleton_method/block_pass.rb",
            "testdata/lsp/code_actions/extract_variable_single/invalid.rb",
            "testdata/lsp/completion/error_recovery_binop_unop.rb",
            "testdata/lsp/completion/if_empty.rb",
            "testdata/lsp/completion/instance_var.rb",
            "testdata/lsp/completion/instance_var_strict.rb",
            "testdata/lsp/completion/method_kwargs.rb",
            "testdata/lsp/completion/method_kwargs_extra.rb",
            "testdata/lsp/completion/missing_assign_rhs.rb",
            "testdata/lsp/completion/operators.rb",
            "testdata/lsp/completion/setter.rb",
            "testdata/lsp/completion/snippet_before_variable.rb",
            "testdata/lsp/completion/superclass.rb",
            "testdata/lsp/completion/symbol_proc.rb",
            "testdata/lsp/completion/untyped_parse_errors.rb",
            "testdata/lsp/completion/untyped_receiver.rb",
            "testdata/lsp/fast_path/alias_stub_module.rb",
            "testdata/lsp/fast_path/call_with_block.rb",
            "testdata/lsp/fast_path/not_enough_files/not_enough_files.rb",
            "testdata/lsp/fast_path/too_many_files/too_many_files.rb",
            "testdata/lsp/hover_it_param.rb",
            "testdata/lsp/hover_methods_inside_block.rb",
            "testdata/lsp/hover_mlhs_assign.rb",
            "testdata/lsp/hover_untyped_lambda.rb",
            "testdata/lsp/prop_factory_jump_to_def.rb",
            "testdata/lsp/prop_references.rb",
            "testdata/lsp/ripper_parse_error_formatting.rb",
            "testdata/lsp/rubyfmt_init_error.rb",
            "testdata/lsp/rubyfmt_io_error_formatting.rb",
            "testdata/lsp/rubyfmt_ruby_error.rb",
            "testdata/lsp/rubyfmt_unknown_error.rb",
            "testdata/lsp/successful_formatting.rb",
            "testdata/lsp/syntax_error_formatting.rb",
            "testdata/lsp/workspace_symbols_stdlib.rb",
            "testdata/minimize-rbi/external.rb",
            "testdata/minimize-rbi/private.rb",
            "testdata/namer/duplicate_field_error.rb",
            "testdata/packager/empty_package_file.rb",
            "testdata/packager/invalid_imports_and_exports.rb",
            "testdata/packager/package-no-spec.rb",
            "testdata/packager/unpackaged-error.rb",
            "testdata/parser/bad_argument_crash.rb",
            "testdata/parser/compare_overload_parse_error.rb",
            "testdata/parser/crash_case_empty_var.rb",
            "testdata/parser/curly_braces_block_pass.rb",
            "testdata/parser/empty.rb",
            "testdata/parser/fuzz_ivar.rb",
            "testdata/parser/it_param_invalid_syntax.rb",
            "testdata/parser/it_param_with_explicit_args.rb",
            "testdata/parser/kwargs_missing_comma_unsupported.rb",
            "testdata/rbi/enumerable.rb",
            "testdata/rbi/proc.rb",
            "testdata/rbs/signatures_defs_abstract_autocorrect.rb",
            "testdata/rbs/signatures_params_autocorrect.rb",
            "testdata/resolver/basic_constant_out_of_order.rb",
            "testdata/resolver/it_param_rspec_method.rb",
            "testdata/resolver/multi_assign.rb",
            "testdata/resolver/sig_misc.rb",
            "testdata/resolver/t_module_subclass.rb",
            "testdata/rewriter/initializer_no_arg_with_local.rb",
            "testdata/rewriter/prop.rb",
            "testdata/rewriter/prop_foreign.rb",

            # Additional failing tests
            "testdata/lsp/fast_path/alias_stub_module__*.rb",
            "testdata/lsp/fast_path/alias_stub_module__*.rbupdate",
            "testdata/lsp/fast_path/not_enough_files/**",
            "testdata/lsp/fast_path/too_many_files/**",
            "testdata/packager/empty_package_file/**",
            "testdata/packager/package-no-spec/**",
            "testdata/packager/unpackaged-error/**",
            "testdata/rbs/signatures_types.rb",
        ],
    ),
    "PrismPosTests",
    parser = "prism",
)

pipeline_tests(
    "whitequark_parser_corpus",
    glob([
        "whitequark/**/*.rb",
        "whitequark/**/*.exp",
    ]),
    "WhitequarkParserTests",
)

pipeline_tests(
    "test_corpus_lsp",
    glob(
        [
            "testdata/**/*.rb",
            "testdata/**/*.rbi",
            "testdata/**/*.exp",
        ],
    ),
    "LSPTests",
    extra_files = ["testdata/lsp/rubyfmt-stub/rubyfmt"],
)

pipeline_tests(
    "prism_regression_corpus",
    glob(
        [
            "prism_regression/**/*.rb",
            "prism_regression/**/*.exp",
        ],
        exclude = [
            # Numbered block parameter location differs: Prism uses last occurrence
            # of `_1`, Whitequark uses first occurrence
            "prism_regression/multiple_numbered_params.rb",

            # related to ruby 3.4 syntax changes
            "prism_regression/assign_to_index.rb",

            # TODO: resolve following: https://github.com/sorbet/sorbet/issues/9630
            "prism_regression/multi_target.rb",
            "prism_regression/multi_write.rb",
        ],
    ),
    "PrismPosTests",
    parser = "prism",
)

test_suite(
    name = "test",
    tests = ["test_corpus"],
)

test_suite(
    name = "test_prism",
    tests = ["test_corpus_prism"],
)

test_suite(
    name = "whitequark_parser_tests",
    tests = ["whitequark_parser_corpus"],
)

test_suite(
    name = "prism_regression",
    tests = ["prism_regression_corpus"],
)
