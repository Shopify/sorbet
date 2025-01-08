genrule(
    name = "copy_dylib",
    srcs = ["@prism_darwin//file:libprism.dylib"],
    outs = ["lib/libprism.dylib"],
    cmd = """
        mkdir -p $(@D)
        cp $< $@
        install_name_tool -id "@loader_path/lib/libprism.dylib" $@
    """,
    visibility = ["//visibility:public"],
)

cc_import(
    name = "prism_darwin",
    hdrs = glob(["prism_headers/**/*.h"]),
    includes = ["prism_headers"],
    shared_library = "lib/libprism.dylib",
    visibility = ["//visibility:public"],
    data = [":copy_dylib"],
    deps = ["//third_party/prism_headers"],
)

