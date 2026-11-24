// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "runtime/instance/module.h"
#include "system/stacktrace.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {

/// Render a recorded stack trace as "module:index" frames. Modules without a
/// registered name are numbered by their order of appearance in the trace.
void dumpStackTrace(Span<const StackTraceEntry> Stack) noexcept {
  std::vector<const Runtime::Instance::ModuleInstance *> Anonymous;
  std::string Out;
  for (size_t I = 0; I < Stack.size(); ++I) {
    if (I != 0) {
      Out += ", "sv;
    }
    const auto *Module = Stack[I].Module;
    std::string_view Name =
        Module ? Module->getModuleName() : std::string_view{};
    if (!Name.empty()) {
      Out += fmt::format("{}:{}"sv, Name, Stack[I].FuncIndex);
    } else {
      auto Iter = std::find(Anonymous.begin(), Anonymous.end(), Module);
      size_t Ordinal;
      if (Iter == Anonymous.end()) {
        Ordinal = Anonymous.size();
        Anonymous.push_back(Module);
      } else {
        Ordinal = static_cast<size_t>(Iter - Anonymous.begin());
      }
      Out += fmt::format("module[{}]:{}"sv, Ordinal, Stack[I].FuncIndex);
    }
  }
  spdlog::error("calling stack:{}"sv, Out);
}

} // namespace

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
  // A number matches by its type, and a reference by its value as it is seen
  // by the type passed with it. The null references are checked below.
  bool IsMatched =
      Params.size() == PTypes.size() && ParamTypes.size() == PTypes.size();
  for (uint32_t I = 0; IsMatched && I < PTypes.size(); ++I) {
    if (!PTypes[I].isRefType()) {
      IsMatched = ParamTypes[I].getCode() == PTypes[I].getCode();
    } else {
      const auto &Ref = Params[I].get<RefVariant>();
      IsMatched = ParamTypes[I].isRefType() &&
                  (Ref.isNull() ||
                   Runtime::Matcher::matchRef(FuncInst->getModule(), PTypes[I],
                                              Ref, ParamTypes[I]));
    }
  }
  if (!IsMatched) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(
        PTypes, RTypes, std::vector(ParamTypes.begin(), ParamTypes.end()),
        RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Check the reference value validation.
  for (uint32_t I = 0; I < PTypes.size(); ++I) {
    if (PTypes[I].isRefType() && (!PTypes[I].isNullableRefType() &&
                                  Params[I].get<RefVariant>().isNull())) {
      spdlog::error(ErrCode::Value::NonNullRequired);
      spdlog::error("    Cannot pass a null reference as argument of {}."sv,
                    PTypes[I]);
      return Unexpect(ErrCode::Value::NonNullRequired);
    }
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  EXPECTED_TRY(runFunction(StackMgr, *FuncInst, Params).map_error([](auto E) {
    if (E != ErrCode::Value::Terminated) {
      dumpStackTrace(
          Span<const StackTraceEntry>{StackTrace}.first(StackTraceSize));
    }
    return E;
  }));

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    auto Val = StackMgr.pop();
    const auto &RType = RTypes[RTypes.size() - I - 1];
    if (RType.isRefType()) {
      // For the reference type cases of the return values, the types should be
      // transformed into abstract heap types due to the opaque of type indices.
      const auto &Ref = Val.get<RefVariant>();
      ValType RefType = Ref.getType();
      if (!Ref.isNull() && RType.isExternRefType()) {
        // The declared type tells that the reference is seen from outside.
        RefType = ValType(TypeCode::Ref, TypeCode::ExternRef);
      } else if (!Ref.isNull() && RefType.isExternRefType()) {
        // A reference to a host object is an any reference.
        RefType = ValType(TypeCode::Ref, TypeCode::AnyRef);
      }
      if (!RefType.isAbsHeapType()) {
        // The instance must not be nullptr because the null references are
        // already dynamic typed into the top abstract heap type.
        auto *Inst = Ref.getPtr<Runtime::Instance::CompositeBase>();
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
      if (!Ref.isNull()) {
        RefType.toNonNullableRef();
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
  EXPECTED_TRY(auto CoreWASMReturns, invoke(CoreFuncInst, CoreWASMArgs,
                                            CoreFuncType.getParamTypes()));

  // Get return values.
  std::vector<ComponentValType> ReturnTypes;
  if (const auto &R = FuncInst->getFuncType().getResult(); R.has_value()) {
    ReturnTypes.push_back(*R);
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
    EXPECTED_TRY(invoke(PostReturnInst, PRArgs,
                        PostReturnInst->getFuncType().getParamTypes()));
  }

  return Returns;
}

/// Stop every active run. See "include/executor/executor.h".
void Executor::stop() noexcept {
  for (auto &L : RunLists) {
    std::unique_lock<std::mutex> Lock(L.Mutex);
    for (auto *Run = L.Head; Run != nullptr; Run = Run->Next) {
      Run->requestStop();
    }
  }
}

namespace {
/// Index of the current thread. Threads take the indices in turn.
size_t threadIndex() noexcept {
  static std::atomic_size_t NextIndex = 0;
  thread_local const size_t Index =
      NextIndex.fetch_add(1, std::memory_order_relaxed);
  return Index;
}
} // namespace

Executor::StopState::StopState(Executor &Target) noexcept
    : Ex(Target),
      List(Target.RunLists[threadIndex() % Target.RunLists.size()]) {
  std::unique_lock<std::mutex> Lock(List.Mutex);
  Next = std::exchange(List.Head, this);
  if (Next != nullptr) {
    Next->Prev = this;
  }
}

Executor::StopState::~StopState() noexcept { unregister(); }

void Executor::StopState::requestStop() noexcept {
  std::unique_lock<std::mutex> Lock(Mutex);
  StopToken.store(1, std::memory_order_relaxed);
  if (Waiter) {
    std::unique_lock<std::mutex> WaiterLock(Waiter->Mutex);
    Waiter->Cond.notify_all();
  }
}

void Executor::StopState::unregister() noexcept {
  if (!Registered) {
    return;
  }
  Registered = false;
  std::unique_lock<std::mutex> Lock(List.Mutex);
  (Prev != nullptr ? Prev->Next : List.Head) = Next;
  if (Next != nullptr) {
    Next->Prev = Prev;
  }
  Prev = Next = nullptr;
}

void Executor::StopState::setWaiter(
    Runtime::Instance::MemoryInstance::Waiter *W) noexcept {
  std::unique_lock<std::mutex> Lock(Mutex);
  Waiter = W;
}

Executor::StopState::Scope::Scope(StopState &R) noexcept
    : Run(R), Saved(std::exchange(CurrentScope, this)) {}

Executor::StopState::Scope::~Scope() noexcept { CurrentScope = Saved; }

Executor::StopState *Executor::enclosingRun() const noexcept {
  return CurrentScope != nullptr && &CurrentScope->Run.Ex == this
             ? &CurrentScope->Run
             : nullptr;
}

Executor::RunScope::RunScope(Executor &Ex) noexcept
    : Run([&]() -> StopState & {
        if (auto *Enclosing = Ex.enclosingRun()) {
          return *Enclosing;
        }
        return LocalRun.emplace(Ex);
      }()),
      Entered(Run) {}

} // namespace Executor
} // namespace WasmEdge
