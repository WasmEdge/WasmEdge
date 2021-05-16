// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/system/allocator.h - large memory allocator --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory allocator for various operating system.
///
//===----------------------------------------------------------------------===//
#pragma once
#include <cstdint>

namespace WasmEdge {

class Allocator {
public:
  static uint8_t *allocate(uint32_t PageCount) noexcept;
  static uint8_t *resize(uint8_t *Pointer, uint32_t OldPageCount,
                         uint32_t NewPageCount) noexcept;
  static void release(uint8_t *Pointer, uint32_t PageCount) noexcept;
};

} // namespace WasmEdge
