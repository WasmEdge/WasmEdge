// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/fuzzPO.h - Fuzz PO entrypoint ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the fuzz PO executable.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstddef>
#include <cstdint>

namespace WasmEdge {
namespace Driver {

int FuzzPO(const uint8_t *Data, size_t Size) noexcept;

} // namespace Driver
} // namespace WasmEdge
