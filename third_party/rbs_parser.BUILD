cc_library(
    name = "rbs_parser",
    srcs = glob(["src/**/*.c"]),
    hdrs = glob(["include/**/*.h"]),
    copts = [
        "-Iexternal/rbs_parser/include",
        "-Wno-error=missing-field-initializers",
        "-Wno-error=implicit-fallthrough",
    ],
    linkopts = [
        "-L/opt/rubies/3.3.3/lib/",
        "/opt/rubies/3.3.3/lib/libruby.3.3.dylib",
    ],
    visibility = ["//visibility:public"],
)
