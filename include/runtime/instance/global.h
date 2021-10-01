// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/runtime/instance/global.h - Global Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const GlobalType &GType,
                 const ValVariant Val = uint32_t(0)) noexcept
      : GlobType(GType), Value(Val) {}
  virtual ~GlobalInstance() = default;

  /// Getter the global value type.
  ValType getValType() const { return GlobType.Type; }

  /// Getter the global mutation.
  ValMut getValMut() const { return GlobType.Mut; }

  /// Getter of value.
  const ValVariant &getValue() const { return Value; }

  /// Getter of value.
  ValVariant &getValue() { return Value; }

private:
  /// \name Data of global instance.
  /// @{
  GlobalType GlobType;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
