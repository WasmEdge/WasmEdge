// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/cputime.h - Process CPU time utility --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the cross-platform process CPU time utility.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace WasmEdge {
namespace CpuTime {

/// Get process CPU time (user + kernel) in nanoseconds.
uint64_t getProcessCpuTimeNs() noexcept;

} // namespace CpuTime
} // namespace WasmEdge
