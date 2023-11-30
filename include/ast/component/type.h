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
// TODO: complete these class
class Record {};
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
