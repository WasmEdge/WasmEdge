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

#include "ast/common.h"
#include "executor/common.h"

namespace SSVM {
namespace Executor {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = default;
  ~GlobalInstance() = default;

  /// Set the global type.
  ErrCode setGlobalType(const AST::ValType &ValueType,
                        const AST::ValMut &Mutibility);

  /// Get the global type.
  AST::ValType getValType() const { return Type; }

  /// Set the value of this instance.
  template <typename T> ErrCode setValue(const T &Val);

  /// Get the value of this instance.
  template <typename T> ErrCode getValue(T &Val);

  /// Global Instance address in store manager.
  unsigned int Addr;

private:
  /// \name Data of global instance.
  /// @{
  AST::ValType Type;
  AST::ValMut Mut;
  AST::ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
