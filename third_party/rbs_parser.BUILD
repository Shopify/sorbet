cc_library(
    name = "rbs_parser",
    srcs = glob(["ext/rbs_extension/*.c", "src/*.c"]),
    hdrs = glob(["ext/rbs_extension/*.h", "include/*.h"]),
    copts = [
        "-Iexternal/rbs_parser/ext/rbs_extension",
        "-Iexternal/rbs_parser/include",
        "-Iexternal/ruby-headers",
        "-Iexternal/ruby-headers2",
        "-Wno-error=missing-field-initializers",
        "-Wno-error=implicit-fallthrough",
    ],
    linkopts = [
        "-L/opt/rubies/3.3.3/lib/",
        "/opt/rubies/3.3.3/lib/libruby.3.3.dylib",
    ],
    deps = [
        "@ruby-headers//:headers",
        "@ruby-headers2//:headers",
    ],
    visibility = ["//visibility:public"],
)
