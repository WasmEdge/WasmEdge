// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::StartSection &StartSec) {
  const auto &Start = StartSec.getContent();

  std::vector<ValInterface> Args;
  for (auto Idx : Start.getArguments()) {
    Args.push_back(CompInst.getValue(Idx));
  }

  auto *FuncInst = CompInst.getFunction(Start.getFunctionIndex());
  const auto &FuncType = FuncInst->getFuncType();
  EXPECTED_TRY(auto ResultList,
               invoke(FuncInst, Args, FuncType.getParamTypes()));
  CompInst.setValue(Start.getResult(), ResultList[0].first);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
