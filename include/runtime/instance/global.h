// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/global.h - Global Instance definition -------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"
#include "common/value.h"

namespace SSVM {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const ValType &ValueType, const ValMut &Mutibility)
      : Type(ValueType), Mut(Mutibility) {
    switch (Type) {
    case ValType::I32:
      Value = (uint32_t)0;
      break;
    case ValType::I64:
      Value = (uint64_t)0;
      break;
    case ValType::F32:
      Value = (float)0.0;
      break;
    case ValType::F64:
      Value = (double)0.0;
      break;
    default:
      break;
    }
  }
  virtual ~GlobalInstance() = default;

  /// Getter the global value type.
  const ValType getValType() const { return Type; }

  /// Getter the global mutation.
  const ValMut getValMut() const { return Mut; }

  /// Getter of value.
  ValVariant &getValue() { return Value; };

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
} // namespace SSVM