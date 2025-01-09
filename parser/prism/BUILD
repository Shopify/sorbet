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
    includes = ["../../third_party/prism_lib/include"], # we get prism.h header not found error without this
    linkstatic = 0,
    visibility = ["//visibility:public"],
    deps = select({
        "@platforms//os:osx": [
            "//third_party/prism_lib",
            "//core",
            "//parser", # Legacy parser, needed for translating to its nodes defined in `parser/Node_gen.h`
        ],
    }),
)
