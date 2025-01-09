cc_library(
    name = "prism",
    srcs = [
        "Parser.cc",
        "Translator.cc",
    ],
    hdrs = [
        "Parser.h",
        "Translator.h",
        "Helpers.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//core",
        "//parser",  # Legacy parser, needed for translating to its nodes defined in `parser/Node_gen.h`
    ] + select({
        "@platforms//os:osx": ["@prism_darwin"],
        "@platforms//os:linux": ["@prism_linux"],
    }),
)
