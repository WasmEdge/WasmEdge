// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  Description(std::string_view V) noexcept : Value(std::move(V)) {}
  std::string_view Value;
};

struct MetaVar {
  MetaVar(std::string_view V) noexcept : Value(std::move(V)) {}
  std::string_view Value;
};

template <typename T> struct DefaultValue {
  DefaultValue(T V) noexcept : Value(std::move(V)) {}
  T Value;
};

struct ZeroOrMore {};
struct OneOrMore {};

struct Hidden {};

} // namespace PO
} // namespace WasmEdge
