// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/type.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

class Start {
  uint32_t FuncIdx;
  std::vector<uint32_t> Args;
  uint32_t Result;

public:
  uint32_t getFunctionIndex() const noexcept { return FuncIdx; }
  uint32_t &getFunctionIndex() noexcept { return FuncIdx; }
  Span<const uint32_t> getArguments() const noexcept { return Args; }
  std::vector<uint32_t> &getArguments() noexcept { return Args; }
  uint32_t getResult() const noexcept { return Result; }
  uint32_t &getResult() noexcept { return Result; }
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
