// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/filesystem.h - std::filesystem selection ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the handling of std::filesystem linkage for various
/// compilers.
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
