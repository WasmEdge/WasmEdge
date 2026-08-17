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
Component::Executor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

/// Register a named Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Component::Executor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp, std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Register an instantiated Component. See
/// "include/executor/component/executor.h".
Expect<void> Component::Executor::registerComponent(
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
Component::Executor::invoke(
    const Runtime::Instance::Component::FunctionInstance *FuncInst,
    Span<const ComponentValVariant> Params,
    Span<const ComponentValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  // TODO: COMPONENT - type matching.
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

  // Reentrance guard: the component model forbids sync reentrance, and this
  // also rejects a call into an instance that is still instantiating.
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
  auto *ReallocFuncInst = FuncInst->getAllocFunction();
  auto *MemInst = FuncInst->getMemoryInstance();
  EXPECTED_TRY(auto CoreWASMArgs,
               convValsToCoreWASM(Params, ParamTypes, ReallocFuncInst, MemInst,
                                  FuncInst->getComponentInstance(),
                                  FuncInst->getStringEncoding()));

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
               convValsToComponent(CoreWASMReturns, ReturnTypes, MemInst,
                                   FuncInst->getComponentInstance(),
                                   FuncInst->getStringEncoding()));
  assuming(Returns.size() == ReturnTypes.size());

  // CanonicalABI.md L3367-3372: after a sync lift completes (post
  // task.return_), invoke the optional post-return with the ORIGINAL flat
  // core return values as parameters. This is how Preview 2 components free
  // buffers allocated for indirect-result / list / string returns.
  //
  // TODO: spec L3370 also gates this region with `may_leave = False`;
  // WasmEdge doesn't model may_leave yet (deferred along with async).
  // In practice sync Preview 2 post-return implementations don't re-enter.
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
