// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/valtype.h - ValueType class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the ValueType related classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace AST {
namespace Component {

// primvaltype ::= 0x7f => bool
//               | 0x7e => s8
//               | 0x7d => u8
//               | 0x7c => s16
//               | 0x7b => u16
//               | 0x7a => s32
//               | 0x79 => u32
//               | 0x78 => s64
//               | 0x77 => u64
//               | 0x76 => f32
//               | 0x75 => f64
//               | 0x74 => char
//               | 0x73 => string
//               | 0x64 => error-context 📝

/// AST Component::PrimValType enum.
enum class PrimValType : uint8_t {
  TypeIndex = 0x00,
  Bool = 0x7f,
  S8 = 0x7e,
  U8 = 0x7d,
  S16 = 0x7c,
  U16 = 0x7b,
  S32 = 0x7a,
  U32 = 0x79,
  S64 = 0x78,
  U64 = 0x77,
  F32 = 0x76,
  F64 = 0x75,
  Char = 0x74,
  String = 0x73,
  ErrorContext = 0x64,
};

// valtype     ::= i:<typeidx>       => i
//               | pvt:<primvaltype> => pvt

// Use the ComponentValType implementation.

// labelvaltype ::= l:<label'> t:<valtype> => l t
// label'       ::= len:<u32> l:<label>    => l (if len = |l|)

/// AST Component::LabelValType definition.
class LabelValType {
public:
  LabelValType() noexcept = default;
  LabelValType(const std::string &L, const ComponentValType &VT) noexcept
      : Label(L), ValTy(VT) {}

  std::string_view getLabel() const noexcept { return Label; }
  void setLabel(const std::string &L) noexcept { Label = L; }
  const ComponentValType &getValType() const noexcept { return ValTy; }
  void setValType(const ComponentValType VT) noexcept { ValTy = VT; }

private:
  std::string Label;
  ComponentValType ValTy;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
