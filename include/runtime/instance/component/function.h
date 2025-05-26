// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/instruction.h"
#include "common/symbol.h"
#include "runtime/component/hostfunc.h"

#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
namespace Component {

class FunctionInstance {
public:
  FunctionInstance() = delete;
  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : FuncType(Inst.FuncType), Data(std::move(Inst.Data)) {}

  FunctionInstance(
      std::unique_ptr<WasmEdge::Runtime::Component::HostFunctionBase>
          &&Func) noexcept
      : FuncType(Func->getFuncType()), Data(std::move(Func)) {}

  const AST::Component::FunctionType &getFuncType() const noexcept {
    return FuncType;
  }

  WasmEdge::Runtime::Component::HostFunctionBase &getHostFunc() const noexcept {
    return *Data;
  }

private:
  AST::Component::FunctionType FuncType;
  std::unique_ptr<WasmEdge::Runtime::Component::HostFunctionBase> Data;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
