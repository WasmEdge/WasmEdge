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
  auto *FuncInst = CompInst.getFunction(Start.getFunctionIndex());
  assuming(FuncInst);
  const auto &FuncType = FuncInst->getFuncType();

  std::vector<ComponentValVariant> Args;
  for (auto Idx : Start.getArguments()) {
    Args.push_back(CompInst.getValue(Idx));
  }
  std::vector<ComponentValType> PTypes;
  for (auto &LType : FuncType.getParamList()) {
    PTypes.push_back(LType.getValType());
  }

  EXPECTED_TRY(auto ResultList, invoke(FuncInst, Args, PTypes));
  CompInst.setValue(Start.getResult(), ResultList[0].first);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
