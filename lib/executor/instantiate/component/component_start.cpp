// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &Comp,
                      const AST::Component::StartSection &StartSec) {
  const auto &Start = StartSec.getContent();

  std::vector<ValInterface> Args;
  for (auto Idx : Start.getArguments()) {
    Args.push_back(Comp.getValue(Idx));
  }

  auto *FuncInst = Comp.getFunctionInstance(Start.getFunctionIndex());
  const auto &FuncType = FuncInst->getFuncType();
  EXPECTED_TRY(auto ResultList,
               invoke(FuncInst, Args, FuncType.getParamTypes()));
  auto Result = ResultList[0].first;
  auto ResultIndex = Start.getResult();
  Comp.setValue(ResultIndex, Result);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
