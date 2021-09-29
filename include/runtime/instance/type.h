// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/runtime/instance/type.h - Function type definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function type definition used in module instance.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/symbol.h"
#include "common/types.h"
#include "common/value.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// Function type definition in this module.
struct FType {
  using Wrapper = AST::FunctionType::Wrapper;

  FType() = default;
  FType(Span<const ValType> P, Span<const ValType> R, Symbol<Wrapper> S)
      : Params(P.begin(), P.end()), Returns(R.begin(), R.end()),
        TypeSymbol(std::move(S)) {}

  friend bool operator==(const FType &LHS, const FType &RHS) noexcept {
    return LHS.Params == RHS.Params && LHS.Returns == RHS.Returns;
  }

  friend bool operator!=(const FType &LHS, const FType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter of symbol
  const auto &getSymbol() const noexcept { return TypeSymbol; }

  std::vector<ValType> Params;
  std::vector<ValType> Returns;

  Symbol<Wrapper> TypeSymbol;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
