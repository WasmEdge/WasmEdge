# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

licenses(["notice"])  # Apache 2

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "srcs",
    srcs = glob(["**"]),
)

# Configuration for WasmEdge with AOT (LLVM) support
cmake(
    name = "wasmedge_lib",
    cache_entries = {
        # Enable LLVM-based AOT compilation
        "WASMEDGE_USE_LLVM": "On",
        "WASMEDGE_BUILD_SHARED_LIB": "Off",
        "WASMEDGE_BUILD_STATIC_LIB": "On",
        "WASMEDGE_BUILD_TOOLS": "Off",
        "WASMEDGE_FORCE_DISABLE_LTO": "On",
        # Link LLVM statically for portability
        "WASMEDGE_LINK_LLVM_STATIC": "On",
        # Provide spdlog and fmt as external dependencies via Bazel (not CMake FetchContent)
        "CMAKE_PREFIX_PATH": "$$EXT_BUILD_DEPS$$/spdlog;$$EXT_BUILD_DEPS$$/fmt;$$EXT_BUILD_DEPS$$/llvm",
        # LLVM configuration
        "LLVM_DIR": "$$EXT_BUILD_DEPS$$/llvm/lib/cmake/llvm",
    },
    env = {
        "CXXFLAGS": "-Wno-error=dangling-reference -Wno-error=maybe-uninitialized -Wno-error=array-bounds -Wno-error=deprecated-declarations -std=c++20",
    },
    generate_args = ["-GNinja"],
    lib_source = ":srcs",
    out_static_libs = ["libwasmedge.a"],
    deps = [
        "@com_github_fmtlib_fmt//:fmt",
        "@com_github_gabime_spdlog//:spdlog",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:ExecutionEngine",
        "@llvm-project//llvm:MCJIT",
        "@llvm-project//llvm:OrcJIT",
        "@llvm-project//llvm:X86CodeGen",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:AArch64CodeGen",
        "@llvm-project//llvm:AArch64AsmParser",
    ],
)

# Alternative configuration without AOT support (interpreter only)
cmake(
    name = "wasmedge_lib_no_aot",
    cache_entries = {
        "WASMEDGE_USE_LLVM": "Off",
        "WASMEDGE_BUILD_SHARED_LIB": "Off",
        "WASMEDGE_BUILD_STATIC_LIB": "On",
        "WASMEDGE_BUILD_TOOLS": "Off",
        "WASMEDGE_FORCE_DISABLE_LTO": "On",
        # Provide spdlog and fmt as external dependencies via Bazel (not CMake FetchContent)
        "CMAKE_PREFIX_PATH": "$$EXT_BUILD_DEPS$$/spdlog;$$EXT_BUILD_DEPS$$/fmt",
    },
    env = {
        "CXXFLAGS": "-Wno-error=dangling-reference -Wno-error=maybe-uninitialized -Wno-error=array-bounds -Wno-error=deprecated-declarations -std=c++20",
    },
    generate_args = ["-GNinja"],
    lib_source = ":srcs",
    out_static_libs = ["libwasmedge.a"],
    deps = [
        "@com_github_fmtlib_fmt//:fmt",
        "@com_github_gabime_spdlog//:spdlog",
    ],
)
