// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

/// Instantiate a Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

/// Register a named Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::registerComponent(Runtime::Component::StoreManager &StoreMgr,
                                     const AST::Component::Component &Comp,
                                     std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Register an instantiated Component. See executor.h.
Expect<void> ComponentExecutor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  return StoreMgr.registerInstance(&CompInst).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  });
}
/// Invoke component function. See "include/executor/component/executor.h".
Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentExecutor::invoke(
    const Runtime::Instance::Component::FunctionInstance *FuncInst,
    Span<const ComponentValVariant> Params,
    Span<const ComponentValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // TODO: COMPONENT - match the arguments against the function type.
  const auto &ExpectedFuncType = FuncInst->getFuncType();
  const size_t ExpectedArity = ExpectedFuncType.getParamList().size();
  if (Params.size() != ParamTypes.size() || ParamTypes.size() < ExpectedArity) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error("    expected {} argument(s), got {}"sv, ExpectedArity,
                  ParamTypes.size());
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Host component functions consume component-level values directly.
  if (FuncInst->isHostFunction()) {
    return FuncInst->getHostFunc()(Params);
  }

  // Reentrance guard: sync reentrance and instantiating instances are out.
  const auto *Parent = FuncInst->getComponentInstance();
  if (Parent != nullptr && Parent->concurrency().entered()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  std::optional<Runtime::Instance::Component::ConcurrencyState::EnterGuard>
      EnterG;
  if (Parent != nullptr) {
    EnterG.emplace(Parent->concurrency());
  }

  // Convert the component params into core WASM params.
  const auto &CanonOpts = FuncInst->getCanonOptions();
  EXPECTED_TRY(auto CoreWASMArgs,
               convValsToCoreWASM(Params, ParamTypes, CanonOpts));

  // Call runFunction.
  auto *CoreFuncInst = FuncInst->getLowerFunction();
  assuming(CoreFuncInst);
  const auto &CoreFuncType = CoreFuncInst->getFuncType();
  // TODO: COMPONENT - check the ABI types between core functype and args.
  EXPECTED_TRY(
      auto CoreWASMReturns,
      core().invoke(CoreFuncInst, CoreWASMArgs, CoreFuncType.getParamTypes()));

  // Get return values.
  std::vector<ComponentValType> ReturnTypes;
  for (const auto &Type : FuncInst->getFuncType().getResultList()) {
    ReturnTypes.push_back(Type.getValType());
  }
  EXPECTED_TRY(auto Returns,
               convValsToComponent(CoreWASMReturns, ReturnTypes, CanonOpts));
  assuming(Returns.size() == ReturnTypes.size());

  // After a sync lift, invoke the optional post-return on the flat results.
  if (auto *PostReturnInst = FuncInst->getPostReturnFunction()) {
    std::vector<ValVariant> PRArgs;
    PRArgs.reserve(CoreWASMReturns.size());
    for (const auto &P : CoreWASMReturns) {
      PRArgs.push_back(P.first);
    }
    EXPECTED_TRY(core().invoke(PostReturnInst, PRArgs,
                               PostReturnInst->getFuncType().getParamTypes()));
  }

  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
