//===-- ssvm/executor/entry/value.h - Value Entry class definition --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Value Entry class in stack manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"
#include "executor/common.h"
#include "support/casting.h"

#include <cstdint>
#include <variant>

namespace SSVM {
namespace Executor {

class ValueEntry;
namespace {
/// Accept Wasm built-in types
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltInV<T>, TR>;
} // namespace

class ValueEntry {
public:
  /// Default constructors for temp ValueEntry.
  ValueEntry() : Type(AST::ValType::I32), Value(0U) {}
  ~ValueEntry() = default;

  /// Initializers of value entry.
  ErrCode InitValueEntry() { return InitValueEntry(0U); }
  ErrCode InitValueEntry(const ValueEntry &VE);
  ErrCode InitValueEntry(const AST::ValType &VT);
  ErrCode InitValueEntry(const AST::ValType &VT, const AST::ValVariant &Val);
  ErrCode InitValueEntry(const AST::ValVariant &Val);
  /// Initializers for the different value types.
  ErrCode InitValueEntry(const uint32_t &Val);
  ErrCode InitValueEntry(const uint64_t &Val);
  ErrCode InitValueEntry(const float &Val);
  ErrCode InitValueEntry(const double &Val);

  /// Getter of value type.
  AST::ValType getType() const { return Type; }

  /// Value setters
  template <typename T> TypeB<T, ErrCode> setValue(const T &Val);
  ErrCode setValue(const ValueEntry &Val);
  ErrCode setValue(const AST::ValVariant &Val);

  /// Getters of getting values.
  template <typename T> TypeB<T, ErrCode> getValue(T &Val) const;
  ErrCode getValue(AST::ValVariant &Val) const;

private:
  /// \name Data of value entry.
  /// @{
  AST::ValType Type;
  AST::ValVariant Value;
  /// @}
};

} // namespace Executor
} // namespace SSVM

#include "value.ipp"