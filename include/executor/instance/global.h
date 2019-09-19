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
#include "executor/instance/entity.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {
namespace Instance {

namespace {
/// Accept Wasm built-in types and variant<...>
template <typename T, typename TR>
using TypeV = typename std::enable_if_t<
    Support::IsWasmBuiltInV<T> || std::is_same_v<T, AST::ValVariant>, TR>;
} // namespace

class GlobalInstance : public Entity {
public:
  GlobalInstance() = delete;
  GlobalInstance(const AST::ValType &ValueType, const AST::ValMut &Mutibility);
  virtual ~GlobalInstance() = default;

  /// Get the global type.
  AST::ValType getValType() const { return Type; }

  /// Set the value of this instance.
  template <typename T> TypeV<T, ErrCode> setValue(const T &Val);

  /// Get the value of this instance.
  template <typename T> TypeV<T, ErrCode> getValue(T &Val) const;

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
