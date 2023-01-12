// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include "common/defines.h"
#include <cstdint>

namespace WasmEdge {

class Allocator {
public:
  static uint8_t *allocate(uint32_t PageCount) noexcept;

  static uint8_t *resize(uint8_t *Pointer, uint32_t OldPageCount,
                         uint32_t NewPageCount) noexcept;

  static void release(uint8_t *Pointer, uint32_t PageCount) noexcept;

  static uint8_t *allocate_chunk(uint64_t Size) noexcept;
  static void release_chunk(uint8_t *Pointer, uint64_t Size) noexcept;
  static bool set_chunk_executable(uint8_t *Pointer, uint64_t Size) noexcept;
  static bool set_chunk_readable(uint8_t *Pointer, uint64_t Size) noexcept;
  static bool set_chunk_readable_writable(uint8_t *Pointer,
                                          uint64_t Size) noexcept;
};

} // namespace WasmEdge
