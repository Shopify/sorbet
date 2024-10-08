cc_library(
    name = "rbs",
    srcs = glob(["*.cc", "*.h"]),
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    deps = [
        "//ast",
        "//common",
        "//core",
        "@rbs_parser//:rbs_parser",
    ],
    copts = [
        "-Iexternal/rbs_parser",
        "-Iexternal/rbs_parser/ext/rbs_extension",
    ],
    visibility = ["//visibility:public"],
)
