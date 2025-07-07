def prism_desugar_test_suite(name, source_files):
    for source_file in source_files:
        # Extract relative path for test naming
        # testdata/desugar/foo/bar.rb -> foo_bar_prism_desugar_test
        relative_path = source_file.replace("testdata/desugar/", "").replace(".rb", "")
        test_name = relative_path.replace("/", "_") + "_prism_desugar_test"
        
        native.sh_test(
            name = test_name,
            srcs = ["prism_desugar_test.sh"],
            args = [source_file],
            tags = ["prism_desugar_test"],
            data = [
                "//main:sorbet",
                "@bazel_tools//tools/bash/runfiles",
                source_file,
            ],
        )
