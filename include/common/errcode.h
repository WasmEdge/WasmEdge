// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/errcode.h - Error code definition -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the `Expected` wrappers.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_errcode.hpp"
#include "common/expected.h"
#include "common/hexstr.h"
#include "common/spdlog.h"

#include <cassert>
#include <ostream>

#if defined(_MSC_VER) && !defined(__clang__)
#define __builtin_unreachable() __assume(0)
#endif

#ifdef NDEBUG
#define assuming(R)                                                            \
  (static_cast<bool>(R) ? static_cast<void>(0) : __builtin_unreachable())
#define assumingUnreachable() __builtin_unreachable()
#else
#define assuming(expr) assert(expr)
#define assumingUnreachable()                                                  \
  (assert(false && "unreachable"), __builtin_unreachable())
#endif

namespace WasmEdge {

static inline constexpr bool likely(bool V) noexcept {
  return __builtin_expect(V, true);
}
static inline constexpr bool unlikely(bool V) noexcept {
  return __builtin_expect(V, false);
}

/// Type aliasing for Expected<T, ErrCode>.
template <typename T> using Expect = Expected<T, ErrCode>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto Unexpect(const ErrCode &Val) { return Unexpected<ErrCode>(Val); }
template <typename... ArgsT> constexpr auto Unexpect(ArgsT... Args) {
  return Unexpected<ErrCode>(ErrCode(Args...));
}
template <typename T> constexpr auto Unexpect(const Expect<T> &Val) {
  return Unexpected<ErrCode>(Val.error());
}

} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ErrCode> : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ErrCode &Code,
         fmt::format_context &Ctx) const noexcept {
    using namespace std::literals;
    std::string Output =
        fmt::format("{} failed: {}, Code: 0x{:03x}"sv, Code.getErrCodePhase(),
                    WasmEdge::ErrCodeStr[Code.getEnum()], Code.getCode());
    return formatter<std::string_view>::format(Output, Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::ErrCode::Value>
    : fmt::formatter<WasmEdge::ErrCode> {
  fmt::format_context::iterator
  format(const WasmEdge::ErrCode::Value &Value,
         fmt::format_context &Ctx) const noexcept {
    return formatter<WasmEdge::ErrCode>::format(WasmEdge::ErrCode(Value), Ctx);
  }
};
