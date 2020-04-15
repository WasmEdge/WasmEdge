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
  GlobalInstance(const ValType ValueType, const ValMut Mutibility,
                 const ValVariant Val = uint32_t(0))
      : Type(ValueType), Mut(Mutibility), Value(Val) {}
  virtual ~GlobalInstance() = default;

  /// Getter the global value type.
  ValType getValType() const { return Type; }

  /// Getter the global mutation.
  ValMut getValMut() const { return Mut; }

  /// Getter of value.
  const ValVariant &getValue() const {
    if (Symbol) {
      return *Symbol;
    } else {
      return Value;
    }
  }

  /// Getter of value.
  ValVariant &getValue() {
    if (Symbol) {
      return *Symbol;
    } else {
      return Value;
    }
  }

  /// Getter of symbol
  void *getSymbol() const { return Symbol; }
  /// Setter of symbol
  void setSymbol(void *S) { Symbol = reinterpret_cast<ValVariant *>(S); }

private:
  /// \name Data of global instance.
  /// @{
  const ValType Type;
  const ValMut Mut;
  ValVariant Value;
  ValVariant *Symbol = nullptr;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
