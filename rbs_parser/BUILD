cc_library(
    name = "rbs_parser",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    copts = [
        "-Iexternal/ruby-headers",
        "-Iexternal/ruby-headers2",
        "-Wno-error=missing-field-initializers",
        "-Wno-error=implicit-fallthrough",
    ],
    deps = [
        "@ruby-headers//:headers",
        "@ruby-headers2//:headers",
    ],
    visibility = ["//visibility:public"],
)
