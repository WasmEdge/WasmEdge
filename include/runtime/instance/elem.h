// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/elem.h - Element Instance definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the element instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ElementInstance {
public:
  ElementInstance() = delete;
  ElementInstance(const uint64_t Offset, const ValType &EType,
                  Span<const RefVariant> Init) noexcept
      : Off(Offset), Type(EType), Refs(Init.begin(), Init.end()) {
    assuming(Type.isRefType());
  }

  /// Get offset in element instance.
  uint64_t getOffset() const noexcept { return Off; }

  /// Get reference type of element instance.
  const ValType &getRefType() const noexcept { return Type; }

  /// Get reference lists in element instance.
  Span<const RefVariant> getRefs() const noexcept { return Refs; }

  /// Clear references in element instance.
  void clear() noexcept {
    Refs.clear();
    Dropped = true;
  }

  bool isDropped() const noexcept { return Dropped; }

private:
  /// \name Data of element instance.
  /// @{
  const uint64_t Off;
  const ValType Type;
  std::vector<RefVariant> Refs;
  bool Dropped = false;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
