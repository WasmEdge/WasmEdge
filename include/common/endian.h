// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/endian.h - endian detect helper -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the detection helper for endian.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "defines.h"

#define WASMEDGE_ENDIAN_LITTLE_BYTE 0

// Windows is always little-endian
#if WASMEDGE_OS_WINDOWS
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif

// macOS including Apple silicon and Intel-based are always little-endian
#if WASMEDGE_OS_MACOS
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif

// We only handled little-endian on linux
#if WASMEDGE_OS_LINUX
#if defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                        \
    defined(__AARCH64EL__) ||                                                  \
    defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif
#endif
