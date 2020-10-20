// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/type.h - Function type definition -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function type definition used in module instance.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/type.h"
#include "common/span.h"
#include "common/types.h"
#include "common/value.h"
#include "loader/symbol.h"

#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

/// Function type definition in this module.
struct FType {
  using Wrapper = AST::FunctionType::Wrapper;

  FType() = default;
  FType(Span<const ValType> P, Span<const ValType> R, DLSymbol<Wrapper> S)
      : Params(P.begin(), P.end()), Returns(R.begin(), R.end()),
        Symbol(std::move(S)) {}

  friend bool operator==(const FType &LHS, const FType &RHS) noexcept {
    return LHS.Params == RHS.Params && LHS.Returns == RHS.Returns;
  }

  friend bool operator!=(const FType &LHS, const FType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter of symbol
  const auto &getSymbol() const noexcept { return Symbol; }
  /// Setter of symbol
  void setSymbol(DLSymbol<Wrapper> S) { Symbol = std::move(S); }

  std::vector<ValType> Params;
  std::vector<ValType> Returns;

  DLSymbol<Wrapper> Symbol;
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
