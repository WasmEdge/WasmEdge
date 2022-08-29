// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_errcode.h - Error code C++ enumerations ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C++ enumerations of WasmEdge error code.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "dense_enum_map.h"
#include "spare_enum_map.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {

/// WasmEdge runtime phasing C++ enumeration class.
enum class WasmPhase : uint8_t {
#define UseWasmPhase
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseWasmPhase
};

static inline constexpr auto WasmPhaseStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<WasmPhase, std::string_view> Array[] = {
#define UseWasmPhase
#define Line(NAME, VALUE, STRING) {WasmPhase::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseWasmPhase
  };
  return DenseEnumMap(Array);
}
();

/// Error category C++ enumeration class.
enum class ErrCategory : uint8_t {
#define UseErrCategory
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCategory
};

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

} // namespace WasmEdge
