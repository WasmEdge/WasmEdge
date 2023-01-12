// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
#include "common/log.h"

#include <cassert>
#include <ostream>

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

static inline WasmPhase getErrCodePhase(const ErrCode &Code) {
  return static_cast<WasmPhase>(Code.getCategory() == ErrCategory::WASM
                                    ? ((Code.getCode() & 0xF0U) >> 5)
                                    : 0x05U // WasmPhase::UserDefined
  );
}

static inline std::ostream &operator<<(std::ostream &OS, const ErrCode &Code) {
  OS << WasmPhaseStr[getErrCodePhase(Code)]
     << " failed: " << ErrCodeStr[Code.getEnum()]
     << ", Code: " << convertUIntToHexStr(Code.getCode(), 2);
  return OS;
}

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

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<WasmEdge::ErrCode::Value> : ostream_formatter {};
#endif
