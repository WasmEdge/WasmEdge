// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

  // The start function is part of this instantiation, so it may enter the
  // instance being built; the instantiation-context task steps aside for
  // the same reason.
  CompInst.setEntered(false);
  ComponentTask *SavedTask = AsyncRt.currentTask();
  AsyncRt.popNestedTask();
  auto Res = invoke(FuncInst, Args, PTypes);
  if (SavedTask != nullptr) {
    AsyncRt.pushNestedTask(SavedTask);
  }
  CompInst.setEntered(true);
  EXPECTED_TRY(auto ResultList, std::move(Res));
  // Start results append to the value index space in declaration order.
  for (uint32_t I = 0; I < Start.getResult() && I < ResultList.size(); ++I) {
    CompInst.addValue(ResultList[I].first);
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ValueSection &ValSec) {
  for (const auto &Value : ValSec.getContent()) {
    // The payloads decode during validation; an empty slot means this AST
    // never passed the validator.
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
