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

cc_library(
    name = "prism_darwin",
    srcs = [":copy_dylib"],
    visibility = ["//visibility:public"],
    linkopts = [
        "-Wl,-rpath,@loader_path/lib",
    ],
    data = [":copy_dylib"],
)
