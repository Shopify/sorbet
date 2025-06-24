def desugar_test_suite(name, srcs):
    for src in srcs:
        test_name = src.replace(".rb", "") + "_desugar_location_test"
        native.sh_test(
            name = test_name,
            srcs = ["desugar_location_test.sh"],
            args = [src],
            tags = ["desugar_location_test"],
            data = [
                "//main:sorbet",
                "@bazel_tools//tools/bash/runfiles",
                src,
            ],
        ) 
