// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;

Expect<void> Executor::instantiate(Runtime::StoreManager &,
                                   Runtime::Instance::ComponentInstance &Comp,
                                   const AST::Component::StartSection &Sec) {
  auto Start = Sec.getContent();

  std::vector<ValInterface> Args{};
  for (auto Idx : Start.getArguments()) {
    auto Res = Comp.getValue(Idx);
    if (!Res) {
      return Unexpect(Res);
    }
    Args.push_back(*Res);
  }

  EXPECTED_TRY(auto *Fn, Comp.getFunctionInstance(Start.getFunctionIndex()));
  auto FnType = Fn->getFuncType();
  EXPECTED_TRY(auto ResultList, invoke(Fn, Args, FnType.getParamTypes()));
  auto Result = ResultList[0].first;
  auto ResultIndex = Start.getResult();
  Comp.setValue(ResultIndex, Result);

  return {};
}

} // namespace Executor
} // namespace WasmEdge
