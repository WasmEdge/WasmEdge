// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Executor {

// Instantiate start section. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::StartSection &StartSec) {
  const auto &Start = StartSec.getContent();
  EXPECTED_TRY(auto *FuncInst, Ctx.getFunction(Start.getFunctionIndex()));
  const auto &FuncType = FuncInst->getFuncType();

  std::vector<ComponentValVariant> Args;
  for (auto Idx : Start.getArguments()) {
    EXPECTED_TRY(const auto *Val, Ctx.getValue(Idx));
    Args.push_back(*Val);
  }
  std::vector<ComponentValType> ParamTypes;
  for (auto &Param : FuncType.getParamList()) {
    ParamTypes.push_back(Param.getValType());
  }

  // The start function enters the instance it is part of instantiating.
  Runtime::Instance::ComponentInstance::EnteredGuard LeaveGuard{
      Ctx.getInstance(), false};
  EXPECTED_TRY(auto ResultList, invoke(FuncInst, Args, ParamTypes));
  // Start results append to the value index space in declaration order.
  for (uint32_t I = 0; I < Start.getResult() && I < ResultList.size(); ++I) {
    Ctx.addValue(ResultList[I].first);
  }
  return {};
}

// Instantiate value section. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::ValueSection &ValSec) {
  for (const auto &Value : ValSec.getContent()) {
    // Payloads decode during validation; an empty slot means it never ran.
    const auto &Cached = Value.getDecoded();
    if (!Cached.has_value()) {
      spdlog::error(ErrCode::Value::NotValidated);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ValueType));
      return Unexpect(ErrCode::Value::NotValidated);
    }
    Ctx.addValue(*Cached);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
