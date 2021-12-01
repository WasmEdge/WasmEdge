// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/aot/version.h - version definition -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the binary version signature of WasmEdge.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace WasmEdge {
namespace AOT {

static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 1;

} // namespace AOT
} // namespace WasmEdge
