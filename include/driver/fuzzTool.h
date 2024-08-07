// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/fuzzTool.h - Fuzz Tool entrypoint -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the fuzz tool executable.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstddef>
#include <cstdint>

namespace WasmEdge {
namespace Driver {

int FuzzTool(const uint8_t *Data, size_t Size) noexcept;

} // namespace Driver
} // namespace WasmEdge
