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
    Args.push_back(Comp.getValue(Idx));
  }

  auto Fn = Comp.getFunctionInstance(Start.getFunctionIndex());
  auto FnType = Fn->getFuncType();
  EXPECTED_TRY(auto ResultList, invoke(Fn, Args, FnType.getParamTypes()));
  auto Result = ResultList[0].first;
  auto ResultIndex = Start.getResult();
  Comp.setValue(ResultIndex, Result);

  return {};
}

} // namespace Executor
} // namespace WasmEdge
