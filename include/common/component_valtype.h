// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/component_valtype.h - Component value type --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the value type of the Component Model: a primitive
/// type code or an index into the type space of a component.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_types.hpp"
#include "common/errcode.h"
#include "common/fmt.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <string_view>

namespace WasmEdge {

// The bit pattern of the component value types:
// ----------------------------------------------------
//  byte |      0th ~ 3rd       |      4th ~ 7th
// ------|----------------------|----------------------
//  code | ComponentTypeCode    |      Type index
// ----------------------------------------------------

/// The primitive value types of the Component Model, at the byte values of
/// their ComponentTypeCode.
enum class PrimValType : uint8_t {
  Bool = static_cast<uint8_t>(ComponentTypeCode::Bool),
  S8 = static_cast<uint8_t>(ComponentTypeCode::S8),
  U8 = static_cast<uint8_t>(ComponentTypeCode::U8),
  S16 = static_cast<uint8_t>(ComponentTypeCode::S16),
  U16 = static_cast<uint8_t>(ComponentTypeCode::U16),
  S32 = static_cast<uint8_t>(ComponentTypeCode::S32),
  U32 = static_cast<uint8_t>(ComponentTypeCode::U32),
  S64 = static_cast<uint8_t>(ComponentTypeCode::S64),
  U64 = static_cast<uint8_t>(ComponentTypeCode::U64),
  F32 = static_cast<uint8_t>(ComponentTypeCode::F32),
  F64 = static_cast<uint8_t>(ComponentTypeCode::F64),
  Char = static_cast<uint8_t>(ComponentTypeCode::Char),
  String = static_cast<uint8_t>(ComponentTypeCode::String),
  ErrorContext = static_cast<uint8_t>(ComponentTypeCode::ErrContext),
};

/// ComponentValType class definition.
class ComponentValType {
public:
  // Note: The padding bytes are reserved and should not be written.
  ComponentValType() noexcept = default;
  // General constructors for initializing data.
  ComponentValType(ComponentTypeCode C, uint32_t I) noexcept {
    Inner.Data.TCode = C;
    Inner.Data.Idx = I;
  }
  ComponentValType(const std::array<uint8_t, 8> R) noexcept {
    std::copy_n(R.cbegin(), 8, Inner.Raw);
  }
  // Constructor for the component value type with primvaltype.
  ComponentValType(ComponentTypeCode C) noexcept {
    Inner.Data.TCode = C;
    Inner.Data.Idx = 0;
    assuming(C != ComponentTypeCode::TypeIndex);
  }
  // Constructor for the component value type with a primitive.
  ComponentValType(PrimValType P) noexcept {
    Inner.Data.TCode = static_cast<ComponentTypeCode>(P);
    Inner.Data.Idx = 0;
  }
  // Constructor for the component value type with type index.
  ComponentValType(uint32_t I) noexcept {
    Inner.Data.TCode = ComponentTypeCode::TypeIndex;
    Inner.Data.Idx = I;
  }

  friend bool operator==(const ComponentValType &LHS,
                         const ComponentValType &RHS) noexcept {
    return (LHS.Inner.Data.TCode == RHS.Inner.Data.TCode) &&
           (LHS.Inner.Data.Idx == RHS.Inner.Data.Idx);
  }
  friend bool operator!=(const ComponentValType &LHS,
                         const ComponentValType &RHS) noexcept {
    return !(LHS == RHS);
  }

  ComponentTypeCode getCode() const noexcept { return Inner.Data.TCode; }
  uint32_t getTypeIndex() const noexcept { return Inner.Data.Idx; }
  const std::array<uint8_t, 8> getRawData() const noexcept {
    std::array<uint8_t, 8> R;
    std::copy_n(Inner.Raw, 8, R.begin());
    return R;
  }

  void setCode(const ComponentTypeCode C) noexcept {
    Inner.Data.TCode = C;
    Inner.Data.Idx = 0;
  }
  void setTypeIndex(const uint32_t I) noexcept {
    Inner.Data.TCode = ComponentTypeCode::TypeIndex;
    Inner.Data.Idx = I;
  }
  bool isPrimValType() const noexcept {
    return Inner.Data.TCode != ComponentTypeCode::TypeIndex;
  }
  /// The primitive this value type names, when isPrimValType().
  PrimValType getPrimValType() const noexcept {
    return static_cast<PrimValType>(Inner.Data.TCode);
  }

private:
  union {
    uint8_t Raw[8];
    struct {
      uint8_t Padding[3];
      ComponentTypeCode TCode;
      uint32_t Idx;
    } Data;
  } Inner;
};

} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ComponentValType>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ComponentValType &Type,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    if (Type.isPrimValType()) {
      fmt::format_to(std::back_inserter(Buffer), "{}"sv,
                     WasmEdge::ComponentTypeCodeStr[Type.getCode()]);
    } else {
      fmt::format_to(std::back_inserter(Buffer), "type[{}]"sv,
                     Type.getTypeIndex());
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
