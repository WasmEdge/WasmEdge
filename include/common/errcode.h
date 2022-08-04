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

#include "common/enum_errcode.h"
#include "common/expected.h"
#include "common/hexstr.h"

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

class ErrCode {
public:
  /// Error code C++ enumeration class.
  enum class Value : uint32_t {
#define UseErrCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCode
  };

  constexpr ErrCategory getCategory() const noexcept {
    return static_cast<ErrCategory>(static_cast<uint8_t>(Inner.Num >> 24));
  }
  constexpr uint32_t getCode() const noexcept {
    return Inner.Num & 0x00FFFFFFU;
  }
  constexpr ErrCode::Value getEnum() const noexcept {
    return getCategory() != ErrCategory::WASM
               ? ErrCode::Value::UserDefError
               : static_cast<ErrCode::Value>(getCode());
  }

  constexpr ErrCode() noexcept : Inner({.Num = 0}) {}
  constexpr ErrCode(const ErrCode &E) noexcept : Inner({.Num = E.Inner.Num}) {}
  constexpr ErrCode(const ErrCode::Value E) noexcept : Inner({.Code = E}) {}
  constexpr ErrCode(const uint32_t N) noexcept
      : Inner({.Num = (N & 0x00FFFFFFU)}) {}
  constexpr ErrCode(const ErrCategory C, const uint32_t N) noexcept
      : Inner({.Num = (static_cast<uint32_t>(C) << 24) + (N & 0x00FFFFFFU)}) {}

  friend constexpr bool operator==(const ErrCode &LHS,
                                   const ErrCode::Value &RHS) noexcept {
    return LHS.Inner.Code == RHS;
  }
  friend constexpr bool operator==(const ErrCode::Value &LHS,
                                   const ErrCode &RHS) noexcept {
    return RHS.Inner.Code == LHS;
  }
  friend constexpr bool operator==(const ErrCode &LHS,
                                   const ErrCode &RHS) noexcept {
    return LHS.Inner.Num == RHS.Inner.Num;
  }
  friend constexpr bool operator!=(const ErrCode &LHS,
                                   const ErrCode::Value &RHS) noexcept {
    return !(LHS == RHS);
  }
  friend constexpr bool operator!=(const ErrCode::Value &LHS,
                                   const ErrCode &RHS) noexcept {
    return !(LHS == RHS);
  }
  friend constexpr bool operator!=(const ErrCode &LHS,
                                   const ErrCode &RHS) noexcept {
    return !(LHS == RHS);
  }
  constexpr ErrCode &operator=(const ErrCode &) noexcept = default;
  constexpr operator uint32_t() const noexcept { return Inner.Num; }

private:
  union {
    uint32_t Num;
    ErrCode::Value Code;
  } Inner;
};

static inline constexpr const auto ErrCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ErrCode::Value, std::string_view> Array[] = {
#define UseErrCode
#define Line(NAME, VALUE, STRING) {ErrCode::Value::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseErrCode
  };
  return SpareEnumMap(Array);
}
();

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
