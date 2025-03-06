// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/array.h - Array Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the array instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/types.h"
#include "gc/allocator.h"
#include "runtime/instance/composite.h"

#include <vector>

namespace WasmEdge::Runtime::Instance {

class ArrayInstance {
public:
  struct RawArray {
    uint32_t Length;
    ValVariant Data[];
  };

  ArrayInstance() = delete;
  ArrayInstance(GC::Allocator &Allocator, const AST::CompositeType &CompType,
                const uint32_t Size, const ValVariant &Init) noexcept;
  ArrayInstance(GC::Allocator &Allocator, const AST::CompositeType &CompType,
                std::vector<ValVariant> &&Init) noexcept;
  ArrayInstance(RawArray *Raw) noexcept : Data(Raw) {}

  /// Get field data in array instance.
  ValVariant &getData(uint32_t Idx) noexcept { return Data->Data[Idx]; }
  const ValVariant &getData(uint32_t Idx) const noexcept {
    return Data->Data[Idx];
  }

  /// Get full array.
  Span<ValVariant> getArray() noexcept { return {Data->Data, Data->Length}; }
  Span<const ValVariant> getArray() const noexcept {
    return {Data->Data, Data->Length};
  }

  /// Get array length.
  uint32_t getLength() const noexcept { return Data->Length; }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return std::max(Data->Length, UINT32_C(1)) - UINT32_C(1);
  }

  RawArray *getRaw() const noexcept { return Data; }

private:
  /// \name Data of array instance.
  /// @{
  RawArray *Data;
  /// @}
};

} // namespace WasmEdge::Runtime::Instance
