// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
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

  // The start function enters the instance it is part of instantiating.
  Runtime::Instance::Component::ConcurrencyState::LeaveGuard LeaveG{
      CompInst.concurrency()};
  auto Res = invoke(FuncInst, Args, PTypes);
  EXPECTED_TRY(auto ResultList, std::move(Res));
  // Start results append to the value index space in declaration order.
  for (uint32_t I = 0; I < Start.getResult() && I < ResultList.size(); ++I) {
    CompInst.addValue(ResultList[I].first);
  }
  return {};
}

Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::ValueSection &ValSec) {
  for (const auto &Value : ValSec.getContent()) {
    // Payloads decode during validation; an empty slot means it never ran.
    const auto &Cached = Value.getDecoded();
    if (!Cached.has_value()) {
      spdlog::error(ErrCode::Value::NotValidated);
      spdlog::error("    value definition has no decoded payload"sv);
      return Unexpect(ErrCode::Value::NotValidated);
    }
    CompInst.addValue(*Cached);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
