// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/helper.h - Helper for Initialize Option -------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include <string>
#include <type_traits>

namespace SSVM {
namespace PO {

struct Description {
  Description(std::string_view Value) noexcept : Value(std::move(Value)) {}
  std::string_view Value;
};

struct MetaVar {
  MetaVar(std::string_view Value) noexcept : Value(std::move(Value)) {}
  std::string_view Value;
};

template <typename T> struct DefaultValue {
  DefaultValue(T Value) noexcept(std::is_nothrow_move_constructible_v<T>)
      : Value(std::move(Value)) {}
  T Value;
};

struct ZeroOrMore {};
struct OneOrMore {};

struct Hidden {};

} // namespace PO
} // namespace SSVM
