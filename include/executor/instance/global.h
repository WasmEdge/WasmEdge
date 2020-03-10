// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/instance/global.h - Global Instance definition ------===//
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
#include "executor/common.h"
#include "executor/instance/entity.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {
namespace Instance {

namespace {
/// Accept Wasm built-in types
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltInV<T>, TR>;
} // namespace

class GlobalInstance : public Entity {
public:
  GlobalInstance() = delete;
  GlobalInstance(const ValType &ValueType, const ValMut &Mutibility);
  virtual ~GlobalInstance() = default;

  /// Get the global type.
  ValType getValType() const { return Type; }

  /// Getter of value. See "include/executor/instance/global.h".
  template <typename T> TypeB<T, ErrCode> getValue(T &Val) const;
  ErrCode getValue(ValVariant &Val) const;

  /// Setter of value. See "include/executor/instance/global.h".
  template <typename T> TypeB<T, ErrCode> setValue(const T &Val);
  ErrCode setValue(const ValVariant &Val);

private:
  /// \name Data of global instance.
  /// @{
  ValType Type;
  ValMut Mut;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM

#include "global.ipp"
