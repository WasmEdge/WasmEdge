// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

//===-- wasmedge/ast/component/instance.h - instance class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instance node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

class InstantiateArg {
public:
  InstantiateArg() noexcept : Name{""}, InstanceIndex{0} {}
  InstantiateArg(std::string_view N, uint32_t Idx) noexcept
      : Name{N}, InstanceIndex{Idx} {}

  std::string_view getName() const noexcept { return Name; }
  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }

private:
  std::string Name;
  uint32_t InstanceIndex;
};

// core:instanceexpr ::= 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
//                          => (instantiate m arg*)
//                     | 0x01 e*:vec(<core:inlineexport>)      => e*
//
// core:sortidx        ::= sort:<core:sort> idx:<u32>
//            => (sort idx)
// core:inlineexport   ::= n:<core:name> si:<core:sortidx>
//      => (export n si)
class CoreInstanceExpr {
public:
  class Instantiate;
  class InlineExports;
};

class CoreInstanceExpr::Instantiate : public CoreInstanceExpr {
public:
  Instantiate(uint32_t Idx, std::vector<InstantiateArg> Args) noexcept
      : ModuleIdx{Idx}, Args{Args} {}

  uint32_t getModuleIdx() const noexcept { return ModuleIdx; }
  Span<const InstantiateArg> getArgs() const noexcept { return Args; }

private:
  uint32_t ModuleIdx;
  std::vector<InstantiateArg> Args;
};

class CoreInstanceExpr::InlineExports : public CoreInstanceExpr {};

} // namespace AST
} // namespace WasmEdge
