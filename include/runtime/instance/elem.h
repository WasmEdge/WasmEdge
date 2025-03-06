// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include "common/errinfo.h"
#include "common/span.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ElementInstance {
public:
  ElementInstance() = delete;
  ElementInstance(const uint32_t Offset, const ValType &EType,
                  Span<const RefVariant> Init) noexcept
      : Off(Offset), Type(EType), Refs(Init.begin(), Init.end()) {
    assuming(Type.isRefType());
  }

  /// Get offset in element instance.
  uint32_t getOffset() const noexcept { return Off; }

  /// Get reference type of element instance.
  const ValType &getRefType() const noexcept { return Type; }

  /// Get reference lists in element instance.
  Span<const RefVariant> getRefs() const noexcept { return Refs; }

  /// Get reference lists in element instance.
  /// Get slice of Refs[Offset : Offset + Length - 1]
  Expect<Span<const RefVariant>> getRefs(uint32_t Offset,
                                         uint32_t Length) const noexcept {
    // Check the accessing boundary.
    if (Offset + Length > Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(
          Offset, Length,
          std::max(static_cast<uint32_t>(Refs.size()), 1U) - 1U));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return Span<const RefVariant>(Refs).subspan(Offset, Length);
  }

  /// Clear references in element instance.
  void clear() { Refs.clear(); }

private:
  /// \name Data of element instance.
  /// @{
  const uint32_t Off;
  const ValType Type;
  std::vector<RefVariant> Refs;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
