// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/defines.h - General defines -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains macro defines that tweak wasmedge internal.
///
//===----------------------------------------------------------------------===//
#pragma once

#if defined(linux) || defined(__linux) || defined(__linux__) ||                \
    defined(__gnu_linux__)

#define WASMEDGE_OS_LINUX 1
#define WASMEDGE_OS_MACOS 0
#define WASMEDGE_OS_WINDOWS 0
#define WASMEDGE_LIB_PREFIX "lib"
#define WASMEDGE_LIB_EXTENSION ".so"

#elif defined(macintosh) || defined(Macintosh) ||                              \
    (defined(__APPLE__) && defined(__MACH__))

#define WASMEDGE_OS_LINUX 0
#define WASMEDGE_OS_MACOS 1
#define WASMEDGE_OS_WINDOWS 0
#define WASMEDGE_LIB_PREFIX "lib"
#define WASMEDGE_LIB_EXTENSION ".dylib"

#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||              \
    defined(__TOS_WIN__) || defined(__WINDOWS__)

#define WASMEDGE_OS_LINUX 0
#define WASMEDGE_OS_MACOS 0
#define WASMEDGE_OS_WINDOWS 1
#define WASMEDGE_LIB_PREFIX ""
#define WASMEDGE_LIB_EXTENSION ".dll"

#else

#error Unsupported environment!

#endif
