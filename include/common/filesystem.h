// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/filesystem.h - std::filesystem selection ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains std::filesystem linkage handling for various
/// compilers and the UTF-8 path conversion helpers.
///
//===----------------------------------------------------------------------===//
#pragma once

#if defined(__cpp_lib_filesystem)
#define EXPERIMENTAL 0
#elif defined(__cpp_lib_experimental_filesystem)
#define EXPERIMENTAL 1
#elif !defined(__has_include)
#define EXPERIMENTAL 1
#elif __has_include(<filesystem>)
#ifdef _MSC_VER
#if __has_include(<yvals_core.h>)
#include <yvals_core.h>
#if defined(_HAS_CXX17) && _HAS_CXX17
#define EXPERIMENTAL 0
#else
#define EXPERIMENTAL 1
#endif
#else
#define EXPERIMENTAL 1
#endif
#else
#define EXPERIMENTAL 0
#endif
#elif __has_include(<experimental/filesystem>)
#define EXPERIMENTAL 1
#else
#error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#endif

#if EXPERIMENTAL
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

#undef EXPERIMENTAL

#include <string>
#include <string_view>

namespace WasmEdge {

/// Build a path from UTF-8 encoded text.
inline std::filesystem::path u8path(std::string_view Source) {
#if defined(__cpp_lib_char8_t)
  return std::filesystem::path(std::u8string(Source.begin(), Source.end()));
#else
  return std::filesystem::u8path(Source);
#endif
}

/// Return the UTF-8 encoded text of a path.
inline std::string u8string(const std::filesystem::path &Path) {
#if defined(__cpp_lib_char8_t)
  const auto Str = Path.u8string();
  return std::string(Str.begin(), Str.end());
#else
  return Path.u8string();
#endif
}

} // namespace WasmEdge
