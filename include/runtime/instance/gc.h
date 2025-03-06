// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/gc.h - GC Instance definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the gc data instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

namespace WasmEdge::Runtime::Instance {

class ModuleInstance;

class GCInstance {
public:
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(push)
#pragma warning(disable : 4200)
#endif
  struct RawData {
    const ModuleInstance *ModInst;
    uint32_t TypeIdx;
    uint32_t Length;
    ValVariant Data[];
  };
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(pop)
#endif

  GCInstance(RawData *Raw) noexcept : Data(Raw) {}

  RawData *getRaw() const noexcept { return Data; }

protected:
  GCInstance() noexcept = default;

  /// \name Data of struct instance.
  /// @{
  RawData *Data = nullptr;
  /// @}
};

} // namespace WasmEdge::Runtime::Instance
