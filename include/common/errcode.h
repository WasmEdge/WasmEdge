// SPDX-License-Identifier: Apache-2.0
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

#include "common/enum_errcode.h"
#include "common/expected.h"
#include "common/hexstr.h"

#include <ostream>

namespace WasmEdge {

static inline WasmPhase getErrCodePhase(ErrCode Code) {
  return static_cast<WasmPhase>((static_cast<uint8_t>(Code) & 0xF0) >> 5);
}

static inline std::ostream &operator<<(std::ostream &OS, const ErrCode Code) {
  OS << WasmPhaseStr[getErrCodePhase(Code)] << " failed: " << ErrCodeStr[Code]
     << ", Code: " << convertUIntToHexStr(static_cast<uint32_t>(Code), 2);
  return OS;
}

static inline constexpr bool likely(bool V) {
  return __builtin_expect(V, true);
}
static inline constexpr bool unlikely(bool V) {
  return __builtin_expect(V, false);
}

/// Type aliasing for Expected<T, ErrCode>.
template <typename T> using Expect = Expected<T, ErrCode>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto Unexpect(const ErrCode &Val) { return Unexpected<ErrCode>(Val); }
template <typename T> constexpr auto Unexpect(const Expect<T> &Val) {
  return Unexpected<ErrCode>(Val.error());
}

} // namespace WasmEdge
