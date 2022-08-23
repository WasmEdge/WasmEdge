// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Start Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Start node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include <vector>
#include <cstdint>

namespace WasmEdge {
namespace AST {

using ValueIdx = uint32_t;

class Start {
public:
  uint32_t getFuncIdx() const { return FuncIdx; }
  void setFuncIdx(uint32_t Val) noexcept { FuncIdx = Val; }

  /// Getter of content.
  Span<const ValueIdx> getArgs() const noexcept { return Args; }
  std::vector<ValueIdx> &getArgs() noexcept { return Args; }

private:
  uint32_t FuncIdx;
  std::vector<ValueIdx> Args;
};

} // namespace AST
} // namespace WasmEdge
