// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#define _TRUE 1
#define _FALSE 0

#define WASMEDGE_ENDIAN_LITTLE_BYTE _FALSE

// Windows is always little-endian
#if defined(WASMEDGE_OS_WINDOWS) && (_TRUE == WASMEDGE_OS_WINDOWS)
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE _TRUE
#endif

// macOS including Apple silicon and Intel-based are always little-endian
#if defined(WASMEDGE_OS_MACOS) && (_TRUE == WASMEDGE_OS_MACOS)
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE _TRUE
#endif

// We only handled little-endian on linux
#if defined(WASMEDGE_OS_LINUX) && (_TRUE == WASMEDGE_OS_LINUX)
#if defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                        \
    defined(__AARCH64EL__) ||                                                  \
    defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE _TRUE
#endif
#endif

#undef _TRUE
#undef _FALSE
