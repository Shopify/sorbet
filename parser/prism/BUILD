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
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//visibility:public"],
    deps = select({
        "@platforms//os:osx": [
            "//third_party/prism_lib",
            "//core",
            "//parser", # Legacy parser, needed for translating to its nodes defined in `parser/Node_gen.h`
        ],
    }),
)
