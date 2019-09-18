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
/// Check the type is built-in types or variant<built-in>.
template <typename T>
inline constexpr const bool IsV =
    Support::IsWasmBuiltInV<T> || std::is_same_v<T, AST::ValVariant>;
/// Check the type is built-in types, variant<built-in>, or ValueEntry.
template <typename T>
inline constexpr const bool IsVE = IsV<T> || std::is_same_v<T, ValueEntry>;

/// Accept Wasm built-in types, and variant<...>.
template <typename T, typename TR>
using TypeV = typename std::enable_if_t<IsV<T>, TR>;
/// Accept Wasm built-in types, variant<...>, and ValueEntry.
template <typename T, typename TR>
using TypeVE = typename std::enable_if_t<IsVE<T>, TR>;
} // namespace

class ValueEntry {
public:
  /// Default constructors for temp ValueEntry.
  ValueEntry() : Type(AST::ValType::I32), Value(0U) {}
  /// Copy constructor for duplication.
  explicit ValueEntry(const ValueEntry &VE) : Type(VE.Type), Value(VE.Value) {}
  explicit ValueEntry(const AST::ValType &VT);
  explicit ValueEntry(const AST::ValType &VT, const AST::ValVariant &Val)
      : Type(VT), Value(Val) {}
  /// Constructors for the different value type
  explicit ValueEntry(const uint32_t &Val)
      : Type(AST::ValType::I32), Value(Val) {}
  explicit ValueEntry(const uint64_t &Val)
      : Type(AST::ValType::I64), Value(Val) {}
  explicit ValueEntry(const float &Val) : Type(AST::ValType::F32), Value(Val) {}
  explicit ValueEntry(const double &Val)
      : Type(AST::ValType::F64), Value(Val) {}

  ~ValueEntry() = default;

  /// Getter of value type.
  AST::ValType getType() const { return Type; }

  /// Value setters
  template <typename T> TypeVE<T, ErrCode> setValue(const T &Val);

  /// Getters of getting values.
  template <typename T> TypeV<T, ErrCode> getValue(T &Val) const;

private:
  /// \name Data of value entry.
  /// @{
  AST::ValType Type;
  AST::ValVariant Value;
  /// @}
};

} // namespace Executor
} // namespace SSVM
