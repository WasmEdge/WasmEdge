// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/start_section.h - Start Section class definitions
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

namespace WasmEdge {
namespace AST {

class StartValueIdx {
public:
  uint32_t getValueIdx() const noexcept { return ValueIdx; }
  void setValueIdx(uint32_t Idx) { ValueIdx = Idx; }

private:
  uint32_t ValueIdx;
};

} // namespace AST
} // namespace WasmEdge
