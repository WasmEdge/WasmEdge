// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/system/mmap.h - Memory mapped file -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory mapped file handler for various operating
/// system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/filesystem.h"

namespace WasmEdge {

class MMap {
public:
  MMap(const std::filesystem::path &Path) noexcept;
  ~MMap() noexcept;
  void *address() const noexcept;
  static bool supported() noexcept;

private:
  void *Handle;
};

} // namespace WasmEdge
