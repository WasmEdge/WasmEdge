// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/fmt.h - {fmt} compatibility helpers ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains helpers for building the WasmEdge fmt::formatter
/// specializations against multiple {fmt} versions.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <fmt/format.h>

#if FMT_VERSION >= 80000
#define WASMEDGE_FMT_CONST const
#else
#define WASMEDGE_FMT_CONST
#endif
