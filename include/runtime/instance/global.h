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
  GlobalInstance(const ValType ValueType, const ValMut Mutibility,
                 const ValVariant Val = uint32_t(0)) noexcept
      : Type(ValueType), Mut(Mutibility), Value(Val) {}
  virtual ~GlobalInstance() = default;

  /// Getter the global value type.
  ValType getValType() const { return Type; }

  /// Getter the global mutation.
  ValMut getValMut() const { return Mut; }

  /// Getter of value.
  const ValVariant &getValue() const { return Value; }

  /// Getter of value.
  ValVariant &getValue() { return Value; }

private:
  /// \name Data of global instance.
  /// @{
  const ValType Type;
  const ValMut Mut;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
