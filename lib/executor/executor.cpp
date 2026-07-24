// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

/// Instantiate a WASM Module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiateModule(Runtime::StoreManager &StoreMgr,
                            const AST::Module &Mod) {
  return instantiate(StoreMgr, Mod).map_error([this](auto E) {
    // If statistics are enabled, dump them here.
    // When an error occurs, subsequent execution will not run.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return E;
  });
}

/// Register a named WASM module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const AST::Module &Mod, std::string_view Name) {
  return instantiate(StoreMgr, Mod, Name).map_error([this](auto E) {
    // If statistics are enabled, dump them here.
    // When an error occurs, subsequent execution will not run.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return E;
  });
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  return StoreMgr.registerModule(&ModInst).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  });
}

/// Register an instantiated module under an alias name.
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst,
                         std::string_view Name) {
  return StoreMgr.registerModule(&ModInst, Name).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  });
}

/// Instantiate a Component. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

/// Register a named Component. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::registerComponent(Runtime::StoreManager &StoreMgr,
                            const AST::Component::Component &Comp,
                            std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Register an instantiated Component. See "include/executor/executor.h".
Expect<void> Executor::registerComponent(
    Runtime::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  return StoreMgr.registerComponent(&CompInst).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  });
}

/// Register a host function which will be invoked before calling a
/// host function.
Expect<void> Executor::registerPreHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPreHost(HostData, HostFunc);
  return {};
}

/// Register a host function which will be invoked after calling a
/// host function.
Expect<void> Executor::registerPostHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPostHost(HostData, HostFunc);
  return {};
}

/// Invoke function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ValVariant, ValType>>>
Executor::invoke(const Runtime::Instance::FunctionInstance *FuncInst,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  const auto &FuncType = FuncInst->getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();
  // The defined type list may be empty if the function is an independent
  // function instance, that is, the module instance will be nullptr. In this
  // case, all value types are number types or abstract heap types.
  //
  // If a function belongs to a component instance, its type should already be
  // converted, so the type list is not needed.
  WasmEdge::Span<const WasmEdge::AST::SubType *const> TypeList = {};
  if (FuncInst->getModule()) {
    TypeList = FuncInst->getModule()->getTypeList();
  }
  if (!AST::TypeMatcher::matchTypes(TypeList, ParamTypes, PTypes)) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(
        PTypes, RTypes, std::vector(ParamTypes.begin(), ParamTypes.end()),
        RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Check the reference value validation.
  for (uint32_t I = 0; I < ParamTypes.size(); ++I) {
    if (ParamTypes[I].isRefType() && (!ParamTypes[I].isNullableRefType() &&
                                      Params[I].get<RefVariant>().isNull())) {
      spdlog::error(ErrCode::Value::NonNullRequired);
      spdlog::error("    Cannot pass a null reference as argument of {}."sv,
                    ParamTypes[I]);
      return Unexpect(ErrCode::Value::NonNullRequired);
    }
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  EXPECTED_TRY(runFunction(StackMgr, *FuncInst, Params).map_error([](auto E) {
    if (E != ErrCode::Value::Terminated) {
      dumpStackTrace(Span<const uint32_t>{StackTrace}.first(StackTraceSize));
    }
    return E;
  }));

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    auto Val = StackMgr.pop();
    const auto &RType = RTypes[RTypes.size() - I - 1];
    if (RType.isRefType()) {
      // For the reference type cases of the return values, they should be
      // transformed into abstract heap types due to the opaque of type indices.
      auto &RefType = Val.get<RefVariant>().getType();
      if (RefType.isExternalized()) {
        // First handle the forced externalized value type case.
        RefType = ValType(TypeCode::Ref, TypeCode::ExternRef);
      }
      if (!RefType.isAbsHeapType()) {
        // The instance must not be nullptr because the null references are
        // already dynamic typed into the top abstract heap type.
        auto *Inst =
            Val.get<RefVariant>().getPtr<Runtime::Instance::CompositeBase>();
        assuming(Inst);
        // The ModInst may be nullptr only in the independent host function
        // instance. Therefore the module instance here must not be nullptr
        // because the independent host function instance cannot be imported and
        // be referred by instructions.
        const auto *ModInst = Inst->getModule();
        const auto *DefType = *ModInst->getType(RefType.getTypeIndex());
        RefType =
            ValType(RefType.getCode(), DefType->getCompositeType().expand());
      }
      // Should use the value type from the reference here due to the dynamic
      // typing rule of the null references.
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RefType);
    } else {
      // For the number type cases of the return values, the unused bits should
      // be erased due to the security issue.
      cleanNumericVal(Val, RType);
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RType);
    }
  }

  // After execution, the value stack size should be 0.
  assuming(StackMgr.size() == 0);
  return Returns;
}

/// Async invoke function. See "include/executor/executor.h".
Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
Executor::asyncInvoke(const Runtime::Instance::FunctionInstance *FuncInst,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (Executor::*FPtr)(
      const Runtime::Instance::FunctionInstance *, Span<const ValVariant>,
      Span<const ValType>) = &Executor::invoke;
  return {FPtr, *this, FuncInst, std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

/// Invoke component function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
Executor::invoke(const Runtime::Instance::Component::FunctionInstance *FuncInst,
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

  const auto *Parent = FuncInst->getComponentInstance();
  // Poisoned instance tree: after any trap inside it, entries trap.
  if (Parent != nullptr && Parent->getRoot()->getConcurrency().Poisoned) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  // Host-entry reentrance: no task of the current chain may already belong
  // to the same instance tree.
  ComponentTask *Caller = AsyncRt.currentTask();
  for (ComponentTask *C = Caller; C != nullptr; C = C->CallerTask) {
    if (Parent != nullptr && C->Inst != nullptr &&
        C->Inst->getRoot() == Parent->getRoot()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      spdlog::error("    cannot enter component instance"sv);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
  }

  // Collect the argument and result plumbing for the task.
  std::vector<ComponentValVariant> ArgVals(Params.begin(), Params.end());
  std::vector<ComponentValType> ReturnTypes;
  for (const auto &Type : ExpectedFuncType.getResultList()) {
    ReturnTypes.push_back(Type.getValType());
  }
  auto Captured =
      std::make_shared<std::optional<std::vector<ComponentValVariant>>>();
  auto OnStart = [ArgVals = std::move(ArgVals)]() {
    return Expect<std::vector<ComponentValVariant>>(ArgVals);
  };
  auto OnResolve =
      [Captured](std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (Results.has_value()) {
      *Captured = std::move(*Results);
    } else {
      // A cancelled host call resolves with no values.
      *Captured = std::vector<ComponentValVariant>{};
    }
    return {};
  };

  // Teardown (aborting parked vehicles) is only safe on the embedder thread:
  // on a vehicle it would join its own thread. A nested invoke propagates the
  // error and lets the outermost embedder invoke tear down.
  const bool OnEmbedder = AsyncRt.currentVehicle() == nullptr;
  AsyncRt.InvokeDepth += 1;
  auto TaskOrErr = componentLiftCall(FuncInst, std::move(OnStart),
                                     std::move(OnResolve), Caller);
  if (!TaskOrErr) {
    AsyncRt.InvokeDepth -= 1;
    if (AsyncRt.InvokeDepth == 0 && OnEmbedder &&
        AsyncRt.trapLatch().has_value()) {
      const auto Err = *AsyncRt.trapLatch();
      AsyncRt.teardown();
      return Unexpect(Err);
    }
    return Unexpect(TaskOrErr.error());
  }
  ComponentTask *T = *TaskOrErr;

  // Async-typed exports: drive the scheduler until the task resolves.
  if (T->FTAsync && T->St != ComponentTask::State::Resolved) {
    auto PumpRes = AsyncRt.pumpUntil(
        [T]() { return T->St == ComponentTask::State::Resolved; });
    if (!PumpRes) {
      AsyncRt.noteTrap(PumpRes.error(), Parent);
      AsyncRt.InvokeDepth -= 1;
      if (AsyncRt.InvokeDepth == 0 && OnEmbedder) {
        const auto Err = AsyncRt.trapLatch().value_or(PumpRes.error());
        AsyncRt.teardown();
        return Unexpect(Err);
      }
      return Unexpect(PumpRes.error());
    }
  }
  AsyncRt.InvokeDepth -= 1;
  // At the outermost embedder invoke, drain any tasks/vehicles this call
  // left behind (e.g. a subtask the guest never dropped) so they do not
  // outlive the call and reference torn-down state later.
  if (AsyncRt.InvokeDepth == 0 && OnEmbedder) {
    AsyncRt.teardown();
  }

  if (!Captured->has_value()) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    spdlog::error("    async-lifted export failed to produce a result"sv);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  auto &Results = **Captured;
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Returns;
  Returns.reserve(Results.size());
  for (size_t I = 0; I < Results.size() && I < ReturnTypes.size(); ++I) {
    Returns.emplace_back(std::move(Results[I]), ReturnTypes[I]);
  }
  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
