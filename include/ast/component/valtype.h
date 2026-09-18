// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

#include "common/component_valtype.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace AST {
namespace Component {

// valtype     ::= i:<typeidx>       => i
//               | pvt:<primvaltype> => pvt

// Use the ComponentValType implementation.

// labelvaltype ::= l:<label'> t:<valtype> => l t
// label'       ::= len:<u32> l:<label>    => l (if len = |l|)

/// AST Component::LabelValType definition.
class LabelValType {
public:
  LabelValType() noexcept = default;
  LabelValType(const ComponentValType &VT) noexcept : Label(""), ValTy(VT) {}
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
