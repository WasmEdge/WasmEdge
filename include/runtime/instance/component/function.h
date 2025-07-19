// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "runtime/instance/component/hostfunc.h"

#include "ast/type.h"
#include "common/symbol.h"

#include <memory>

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

  FunctionInstance(std::unique_ptr<HostFunctionBase> &&Func) noexcept
      : FuncType(Func->getFuncType()), Data(std::move(Func)) {}

  const AST::FunctionType &getFuncType() const noexcept { return FuncType; }

  HostFunctionBase &getHostFunc() const noexcept { return *Data; }

private:
  AST::FunctionType FuncType;
  std::unique_ptr<HostFunctionBase> Data;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
