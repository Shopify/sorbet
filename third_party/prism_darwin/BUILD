cc_import(
    name = "prism_darwin",
    hdrs = glob(["prism_headers/**/*.h"]),
    includes = ["prism_headers"],
    shared_library = "@prism_darwin_dylib//file:libprism.dylib",
    visibility = ["//visibility:public"],
    deps = ["//third_party/prism_headers"],
)
