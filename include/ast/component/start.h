// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/start.h - Start class definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Start node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/span.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// start ::= f:<funcidx> arg*:vec(<valueidx>) r:<u32>
//         => (start f (value arg)* (result (value))ʳ)

/// AST Component::Start node.
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
