// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/fuzzWAT.h - Fuzz WAT entrypoint -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Entrypoint for the WAT fuzz executable.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstddef>
#include <cstdint>

namespace WasmEdge {
namespace Driver {

int FuzzWAT(const uint8_t *Data, size_t Size) noexcept;

} // namespace Driver
} // namespace WasmEdge
