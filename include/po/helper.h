// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/po/helper.h - Helper for Initialize Option ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include <string>
#include <type_traits>

namespace WasmEdge {
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
  DefaultValue(T Value) noexcept : Value(std::move(Value)) {}
  T Value;
};

struct ZeroOrMore {};
struct OneOrMore {};

struct Hidden {};

} // namespace PO
} // namespace WasmEdge
