// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

//===-- wasmedge/ast/component/type.h - type class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Type node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

enum class PrimValType {
  Bool,
  S8,
  U8,
  S16,
  U16,
  S32,
  U32,
  S64,
  U64,
  Float32,
  Float64,
  Char,
  String
};

using TypeIndex = uint32_t;
using ValueType = std::variant<PrimValType, TypeIndex>;

class LabelValType {
public:
  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  ValueType ValTy;
};

class Record {
public:
  Span<const LabelValType> getLabelTypes() const noexcept { return LabelTypes; }
  std::vector<LabelValType> &getLabelTypes() noexcept { return LabelTypes; }

private:
  std::vector<LabelValType> LabelTypes;
};

// TODO: complete these class
class VariantTy {};
class List {};
class Tuple {};
class Flags {};
class Enum {};
class Option {};
class Result {};
class Own {};
class Borrow {};
using DefValType = std::variant<PrimValType, Record, VariantTy, List, Tuple,
                                Flags, Enum, Option, Result, Own, Borrow>;

// TODO: complete these class
class FuncType {
  // functype ::= 0x40 ps:<paramlist> rs:<resultlist>  => (func ps rs)
};
class ComponentType {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>)   => (component cd*)
};
class InstanceType {
  // instancetype  ::= 0x42 id*:vec(<instancedecl>)    => (instance id*)
};

using DefType = std::variant<DefValType, FuncType, ComponentType, InstanceType>;

} // namespace AST
} // namespace WasmEdge
