# Platform #

platform(
    name = "apple-silicon",
    constraint_values = [
        "@platforms//cpu:arm64",
        "@platforms//os:osx",
    ]
)

platform(
    name = "apple-x86",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:osx",
    ],
)

load("@llvm_toolchain_12_0_0//:cc_toolchain_config.bzl", "cc_toolchain_config")
load("@com_grail_bazel_toolchain//toolchain/internal:sysroot.bzl", "process_sysroot")

cc_toolchain_config(
    name = "clang-darwin-arm64-config",
    host_platform = "darwin",
    custom_target_triple = "aarch64-apple-darwin",
    overrides = {
        "target_system_name": "aarch64-apple-darwin",
        "target_cpu": "aarch64",
        "target_libc": "darwin_aarch64",
        "abi_libc_version": "darwin_aarch64",
        "sysroot_path": process_sysroot("@macos-11.3-sdk//:sysroot")[0],
        "extra_linker_flags": ["-fuse-ld=lld"],
        "omit_cxx_stdlib_flag": False,
        # "use_llvm_ar_instead_of_libtool_on_macos": True,
    },
)

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "conditional_cc_toolchain")
conditional_cc_toolchain(
    name = "clang-darwin-arm64-toolchain",
    toolchain_config = ":clang-darwin-arm64-config",
    host_is_darwin = True,
    sysroot_label = "@macos-11.3-sdk//:sysroot",
    llvm_repo_label_prefix = "@llvm_toolchain_12_0_0//",
)

toolchain(
    name = "clang-darwin-arm64",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:osx",
    ],
    target_compatible_with = [
        "@platforms//cpu:arm64",
        "@platforms//os:osx",
    ],
    toolchain = ":clang-darwin-arm64-toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
