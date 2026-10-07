// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "runtime/instance/gc.h"
#include "runtime/instance/module.h"
#include "system/stacktrace.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
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

/// Attach + validate a prebuilt module's owned GC roots before publishing it.
/// See "include/executor/executor.h".
Expect<RegisteredRootsClaim> Executor::attachRegisteredModuleRoots(
    const Runtime::Instance::ModuleInstance &ModInst,
    bool IncludeImports) noexcept {
  // Only a GC-enabled executor takes ownership of a prebuilt module's roots. A
  // non-GC executor has no live collector to scan them and registers exactly as
  // before -- no walk, no attach. An empty (disarmed) claim rolls back nothing
  // and commits nothing.
  if (!GCEnabled) {
    return RegisteredRootsClaim{};
  }
  GC::Allocator &Alloc = getAllocator();

  // Record every instance this call took a hold on (the ones it attached and
  // the ones it found already attached to this allocator) and nothing else; see
  // RegisteredRootsClaim::recordTable. On the first rejection the claim goes
  // out of scope and releases exactly the recorded holds. Releasing a hold
  // detaches the root only when it was the last one (TableInstance::
  // AttachRefs), so a registration that fails cannot un-root a module another
  // registration, or an instantiation, published.
  //
  // Globals that cannot hold a managed object (numeric ones -- the bulk of a
  // typical module's globals) are skipped before any lock: they are freely
  // shareable and the walk must stay cheap on invoke's hot path. Every table
  // goes through claimForRegister, because a table has a second hazard that
  // does not depend on its element type: a growable table owned by a foreign
  // controller retires its old buffer at that controller's stop-the-world,
  // which never parks this executor's mutators (see TableInstance::
  // setAllocatorLocked). A core ModuleInstance has no nested-component table
  // graph.
  //
  // The index space is read under the module mutex, shared: every mutator
  // that extends it (addHost*, import*, add*) pushes into these vectors under
  // the same mutex held exclusively, and a module stays open to those until
  // it is finalized. The per-root claims below take each instance's own lock
  // inside it, the order the mutators already use (addTable attaches the new
  // table under the module lock).
  std::shared_lock ModLock(ModInst.Mutex);
  RegisteredRootsClaim Claim(Alloc);
  // Read under the same lock as the walk, so the stamp decision (made after
  // the lock is gone, once publication succeeded) describes exactly the index
  // space the walk saw: the generation the stamp is checked against, and
  // whether owned roots are the whole index space. Both are plain fields the
  // mutators write under the exclusive lock.
  Claim.noteWalk(ModInst.GCRootsGeneration.load(std::memory_order_acquire),
                 IncludeImports ||
                     (ModInst.TabInsts.size() == ModInst.OwnedTabInsts.size() &&
                      ModInst.ImpGlobalNum == 0));

  auto ClaimTable = [&](Runtime::Instance::TableInstance *Tab) -> Expect<void> {
    if (Tab->canHoldManaged() || Tab->isGrowable()) {
      Claim.noteAllocatorSpecificRoot();
    }
    EXPECTED_TRY(bool Claimed, Tab->claimForRegister(Alloc));
    if (Claimed) {
      Claim.recordTable(*Tab);
    }
    return {};
  };
  auto ClaimGlobal =
      [&](Runtime::Instance::GlobalInstance *Glob) -> Expect<void> {
    if (!Glob->canHoldManaged()) {
      return {};
    }
    Claim.noteAllocatorSpecificRoot();
    EXPECTED_TRY(bool Claimed, Glob->claimForRegister(Alloc));
    if (Claimed) {
      Claim.recordGlobal(*Glob);
    }
    return {};
  };
  if (IncludeImports) {
    for (auto *Tab : ModInst.TabInsts) {
      EXPECTED_TRY(ClaimTable(Tab));
    }
    for (auto *Glob : ModInst.GlobInsts) {
      EXPECTED_TRY(ClaimGlobal(Glob));
    }
  } else {
    for (const auto &OwnedTab : ModInst.OwnedTabInsts) {
      EXPECTED_TRY(ClaimTable(OwnedTab.get()));
    }
    for (const auto &OwnedGlob : ModInst.OwnedGlobInsts) {
      EXPECTED_TRY(ClaimGlobal(OwnedGlob.get()));
    }
  }
  return Claim;
}

Expect<void> Executor::claimCalleeModuleRoots(
    const Runtime::Instance::ModuleInstance &ModInst) noexcept {
  // The callee's tables and globals must be roots of this executor's
  // allocator: a module another executor instantiated (or a prebuilt one never
  // registered here) has its managed-capable roots attached to a foreign
  // allocator, so a struct.new allocated here and stored into such a global is
  // barriered against the wrong allocator and rooted by nobody this collector
  // scans, and the next collection frees it under the guest.
  //
  // Fast path: a module stamped with this allocator (instantiated here, or
  // claimed by an earlier call/registerModule), or stamped shared because it
  // has no allocator-specific root, has already passed the walk, and
  // ownership never moves away from a live allocator, so only the first touch
  // pays for the walk and its per-instance locks. Every index-space mutator
  // drops the stamp, so a root added later is walked too.
  if (!GCEnabled || ModInst.gcRootsOwnedBy(getAllocator().getId())) {
    return {};
  }
  // The walk holds the module mutex shared, so no root can be added while it
  // runs; a root added between the walk and the stamp would still make the
  // stamp claim more than the walk saw, which the generation check refuses.
  // A refused stamp is harmless here: the roots the walk did attach are ours
  // either way, and a callee runs only once its module is finalized
  // (admitHostModule finalizes a host module and then repeats this check), so
  // the walk that admits the call always covers the final index space.
  auto Claim = attachRegisteredModuleRoots(ModInst, /*IncludeImports=*/true);
  if (!Claim) {
    spdlog::error(Claim.error());
    return Unexpect(Claim.error());
  }
  Claim->commit();
  ModInst.stampGCRootsOwner(stampIdFor(*Claim), Claim->generation());
  return {};
}

Expect<void> Executor::claimCalleeModuleRootsOrLog(
    const Runtime::Instance::ModuleInstance &ModInst,
    std::string_view Detail) noexcept {
  return claimCalleeModuleRoots(ModInst).map_error([Detail](auto E) {
    spdlog::error(Detail);
    return E;
  });
}

template <typename PublishT>
Expect<void> Executor::registerModuleWithRoots(
    const Runtime::Instance::ModuleInstance &ModInst, PublishT &&Publish) {
  // Take ownership of the prebuilt module's GC roots (and reject an unsafe
  // cross-controller share) before publishing it.
  auto Claim = attachRegisteredModuleRoots(ModInst);
  if (!Claim) {
    spdlog::error(Claim.error());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Claim.error());
  }
  EXPECTED_TRY(Publish().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));
  // Published: keep the ownership claim. If the publication fails, the
  // EXPECTED_TRY above returns without committing, and the claim's destructor
  // reverses it. The walk covered only the module's own roots, so the stamp
  // (which invoke reads as "the full index space is ours") is set only when
  // the module imports nothing rootable, as the walk saw under its lock;
  // otherwise invoke's first call completes the claim and stamps. The
  // generation check refuses the stamp if the index space moved since the
  // walk.
  Claim->commit();
  if (GCEnabled && Claim->coversFullIndexSpace()) {
    ModInst.stampGCRootsOwner(stampIdFor(*Claim), Claim->generation());
  }
  return {};
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  return registerModuleWithRoots(
      ModInst, [&]() { return StoreMgr.registerModule(&ModInst); });
}

/// Register an instantiated module under an alias name.
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst,
                         std::string_view Name) {
  return registerModuleWithRoots(
      ModInst, [&]() { return StoreMgr.registerModule(&ModInst, Name); });
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

  // Operation lease for the whole synchronous public invocation: held from
  // before the StackManager below registers this thread's value stack until
  // after the returns are built and the stack deregisters (function scope).
  // Acquisition is serialized with the teardown drain and refused once
  // Closing, so (a) an invocation can never register a fresh stack behind a
  // drain that already observed RegisteredStacks == 0 -- registerStack itself
  // has no error channel (StackManager's ctor is noexcept), the lease is that
  // channel -- and (b) teardown never completes while this call still uses
  // executor state.
  auto OpLease = getController().acquireLease();
  if (unlikely(!OpLease.valid())) {
    spdlog::error(ErrCode::Value::Interrupted);
    spdlog::error("    executor is shutting down; invocation refused"sv);
    return Unexpect(ErrCode::Value::Interrupted);
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

  // GC root ownership of the entry module. enterFunction repeats this check at
  // every module switch below; doing it here too refuses a foreign module
  // before any stack is built and with the invoke-specific diagnostic.
  if (const auto *ModInst = FuncInst->getModule(); ModInst != nullptr) {
    EXPECTED_TRY(claimCalleeModuleRootsOrLog(
        *ModInst, "    The module of the invoked function has GC roots "
                  "(tables/globals) owned by another executor. Invoke it "
                  "through the executor that instantiated it."sv));
  }

  Runtime::StackManager StackMgr(getController());

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
    // Peek, don't pop: the retain below must run while the value is still on
    // the GC-rooted value stack. Popping first would leave the object reachable
    // only via this thread's native-stack local, which another thread's
    // collector does not scan -- it could be swept before retainResult.
    auto Val = StackMgr.peekTop<ValVariant>();
    const auto &RType = RTypes[RTypes.size() - I - 1];
    if (RType.isRefType()) {
      // For the reference type cases of the return values, they should be
      // transformed into abstract heap types due to the opaque of type indices.
      auto &RefType = Val.get<RefVariant>().getType();
      // An externalized reference (extern.convert_any) is handed to the host as
      // externref, but a wrapped GC struct/array still must be retained.
      // Resolve its real heap kind first and fold to externref only after the
      // retain decision: folding first discards the concrete type, and the
      // object would be handed over unrooted and swept. Externalization only
      // sets a flag, leaving the type index and RawData payload intact.
      const bool Externalized = RefType.isExternalized();
      if (!RefType.isAbsHeapType()) {
        // Null references are dynamic-typed into the top abstract heap type,
        // so the reference here is non-null.
        const auto *ModInst = definingModule(Val.get<RefVariant>());
        // The ModInst may be nullptr only in the independent host function
        // instance. Therefore the module instance here must not be nullptr
        // because the independent host function instance cannot be imported and
        // be referred by instructions.
        assuming(ModInst);
        const auto *DefType = *ModInst->getType(RefType.getTypeIndex());
        RefType =
            ValType(RefType.getCode(), DefType->getCompositeType().expand());
      }
      // Retain GC-managed (struct/array) refs returned to the host so the
      // collector keeps them alive until released (null and i31 excluded).
      // The value is still on the stack, so it stays rooted until it lands in
      // HostRoots. An externalized struct/array is retained but handed over
      // typed as externref, so only releaseAllRefs frees it: the by-value
      // releaseRef type filter excludes externref.
      const auto &RetRef = Val.get<RefVariant>();
      if (!RetRef.isNull() && RefType.isHostRetainedRefType()) {
        getAllocator().retainResult(RetRef);
      }
      // Retain decision done; present an externalized ref as plain externref.
      if (Externalized) {
        RefType = ValType(TypeCode::Ref, TypeCode::ExternRef);
      }
      StackMgr.pop<ValVariant>();
      // Should use the value type from the reference here due to the dynamic
      // typing rule of the null references.
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RefType);
    } else {
      StackMgr.pop<ValVariant>();
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

namespace {
/// Keep-alive bundle carried into an asyncInvoke worker as the Async's opaque
/// keep-alive. It holds, for the whole invocation, (a) boundary roots pinning
/// the managed ref parameters and (b) a dependency pin on the function's
/// defining module. Both are released when the worker destroys the keep-alive
/// -- on the worker thread, once the invocation has returned but before the
/// result is published, so this is correct even if the caller drops the Async
/// handle immediately (fire-and-forget) and a caller woken by get() may destroy
/// the defining module at once.
struct AsyncInvokeKeepAlive {
  GC::BoundaryRoots ParamRoots;
  Runtime::Instance::ModulePin ModulePin;
  AsyncInvokeKeepAlive(GC::Allocator &Alloc,
                       const Runtime::Instance::ModuleInstance *Mod) noexcept
      : ParamRoots(Alloc), ModulePin(Mod) {}
};
} // namespace

/// Async invoke function. See "include/executor/executor.h".
Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
Executor::asyncInvoke(const Runtime::Instance::FunctionInstance *FuncInst,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  // Hold a launch lease across the whole preparation window before touching
  // any GC state. Building the keep-alive dereferences the raw target, takes
  // the module pin, and pins the parameter roots into the allocator; a
  // concurrent teardown that observed no outstanding lease could otherwise
  // complete its drain and free the allocator/target underneath this
  // preparation. The Async constructor acquires its own worker lease before
  // this one is released on return, so the target stays pinned with no gap. A
  // refused lease (the controller is closing) launches nothing and returns an
  // invalid Async, matching the Async constructor's own refusal.
  auto PrepLease = getController().acquireLease();
  if (!PrepLease.valid()) {
    return {};
  }
  // The detached worker dereferences FuncInst and its defining module for the
  // whole invocation, protected only by the module pin, which can defer
  // deletion only for a heap module torn down via terminate(). Refuse a target
  // that cannot be pinned: a null module (independent host function) has
  // nothing to pin, and a stack/member (embedder-constructed) module cannot be
  // deferred. Either would leave the worker reading storage a caller may
  // destroy mid-flight.
  if (FuncInst == nullptr || FuncInst->getModule() == nullptr) {
    spdlog::error(
        "asyncInvoke: the target has no defining module (independent host "
        "function), so an async invocation cannot pin it; invoke it "
        "synchronously"sv);
    return {};
  }
  if (!FuncInst->getModule()->isDeferrableStorage()) {
    spdlog::error(
        "asyncInvoke: the target's defining module is not heap/terminate()-"
        "managed, so an async invocation cannot pin it; invoke synchronously "
        "or instantiate the module via the runtime"sv);
    return {};
  }
  // Build the worker keep-alive before detaching and carry it into the worker
  // through the Async, so it is released only after the invocation returns.
  // Its boundary roots pin the managed ref parameters, which until the worker
  // pushes them onto its registered stack live only in the detached argument
  // tuple that no collector scans.
  auto Keep = std::make_shared<AsyncInvokeKeepAlive>(getAllocator(),
                                                     FuncInst->getModule());
  GC::pinParamRootsInto(Keep->ParamRoots, Params, ParamTypes);
  Expect<std::vector<std::pair<ValVariant, ValType>>> (Executor::*FPtr)(
      const Runtime::Instance::FunctionInstance *, Span<const ValVariant>,
      Span<const ValType>) = &Executor::invoke;
  return {std::move(Keep),
          FPtr,
          *this,
          FuncInst,
          std::vector(Params.begin(), Params.end()),
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

} // namespace Executor
} // namespace WasmEdge
