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
Expect<Executor::RegisteredRootsClaim> Executor::attachRegisteredModuleRoots(
    const Runtime::Instance::ModuleInstance &ModInst) noexcept {
  // Only a GC-enabled executor takes ownership of a prebuilt module's roots. A
  // non-GC executor has no live collector to scan them and registers exactly as
  // before -- no walk, no attach. An empty (disarmed) claim rolls back nothing
  // and commits nothing.
  if (!GCEnabled) {
    return RegisteredRootsClaim{};
  }
  GC::Allocator &Alloc = getAllocator();

  // Preflight-all-then-commit with rollback: claim every owned table and global
  // for THIS allocator, validating the two cross-controller sharing hazards.
  // Record only the instances this call newly claimed, so a rollback never
  // detaches an already same-owner or freely-shareable one. On the first
  // rejection, reverse exactly the recorded attaches and return the error
  // without publishing. The returned claim stays armed so the caller also
  // reverses them if publication fails.
  //
  // Only the module's own owned tables and globals are walked: imported ones
  // belong to another module whose registration already attached them, and a
  // core ModuleInstance has no nested-component table graph.
  RegisteredRootsClaim Claim(Alloc);

  for (const auto &OwnedTab : ModInst.OwnedTabInsts) {
    auto *Tab = OwnedTab.get();
    auto Claimed = Tab->claimForRegister(Alloc);
    if (!Claimed) {
      return Unexpect(Claimed.error());
    }
    if (*Claimed) {
      Claim.recordTable(*Tab);
    }
  }
  for (const auto &OwnedGlob : ModInst.OwnedGlobInsts) {
    auto *Glob = OwnedGlob.get();
    auto Claimed = Glob->claimForRegister(Alloc);
    if (!Claimed) {
      return Unexpect(Claimed.error());
    }
    if (*Claimed) {
      Claim.recordGlobal(*Glob);
    }
  }
  return Claim;
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  // Take ownership of the prebuilt module's GC roots (and reject an unsafe
  // cross-controller share) before publishing it.
  auto Claim = attachRegisteredModuleRoots(ModInst);
  if (!Claim) {
    spdlog::error(Claim.error());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Claim.error());
  }
  EXPECTED_TRY(StoreMgr.registerModule(&ModInst).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));
  // Published: keep the ownership claim. Returning without committing (the
  // rejected-publication path above) reverses it.
  Claim->commit();
  return {};
}

/// Register an instantiated module under an alias name.
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst,
                         std::string_view Name) {
  // Take ownership of the prebuilt module's GC roots (and reject an unsafe
  // cross-controller share) before publishing it under the alias name.
  auto Claim = attachRegisteredModuleRoots(ModInst);
  if (!Claim) {
    spdlog::error(Claim.error());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Claim.error());
  }
  EXPECTED_TRY(StoreMgr.registerModule(&ModInst, Name).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));
  // Published: keep the ownership claim. Returning without committing (the
  // rejected-publication path above) reverses it.
  Claim->commit();
  return {};
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
      // Resolve its real heap kind FIRST and fold to externref only AFTER the
      // retain decision -- folding first discards the concrete type and the
      // object would be handed over unrooted and swept. Externalization only
      // sets a flag, leaving the type index and RawData payload intact.
      const bool Externalized = RefType.isExternalized();
      if (!RefType.isAbsHeapType()) {
        // Resolve the defining module from the leading `const ModuleInstance *`
        // of the payload (see getInnerPtr). Null references are dynamic-typed
        // into the top abstract heap type, so the payload here is non-null.
        const auto *ModInst =
            Val.get<RefVariant>()
                .getInnerPtr<const Runtime::Instance::ModuleInstance>();
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
      // Still on the stack, so the object stays rooted until it lands in
      // HostRoots -- no collectible window.
      // NOTE: an externalized struct/array is retained but handed over typed as
      // externref, so it can currently be freed only via releaseAllRefs, not
      // by-value releaseRef (whose type filter excludes externref) -- a known
      // limitation of the C-API release contract.
      const auto &RetRef = Val.get<RefVariant>();
      if (!RetRef.isNull() && RefType.isGCRefType()) {
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
  // Build the worker keep-alive BEFORE detaching, then carry it into the worker
  // via the Async so it is released only after the invocation returns.
  //
  //  * Boundary roots pin managed ref parameters. Until the worker pushes them
  //    onto its registered stack they live only in the detached argument tuple,
  //    which no collector scans -- another mutator could reclaim a parameter
  //    that is the caller's last root.
  //  * The module pin keeps the function's externally-owned defining module
  //    alive. The controller launch lease only blocks Executor/Controller
  //    teardown; it cannot defer a store/module the caller owns. A heap module
  //    torn down via terminate() while the worker runs is now deferred until
  //    the worker releases this pin (a stack/member module still must outlive
  //    the invocation -- the pin turns a caller that destroys it early into a
  //    detected dependents-outstanding abort rather than a silent UAF).
  // Hold a launch lease across the whole preparation window BEFORE touching any
  // GC state. Building the keep-alive dereferences the raw target, takes the
  // module pin, and pins the parameter roots into the allocator; a concurrent
  // teardown that observed no outstanding lease could otherwise complete its
  // drain and free the allocator/target underneath this preparation. The Async
  // constructor acquires its own worker lease before this one is released on
  // return, so the target stays pinned with no gap. If the controller is
  // already closing the lease is refused (empty/invalid): launch nothing and
  // return an invalid Async, matching the Async constructor's own refusal.
  auto PrepLease = getController().acquireLease();
  if (!PrepLease.valid()) {
    return {};
  }
  // The detached worker dereferences FuncInst and its defining module for the
  // whole invocation, protected only by the ModulePin below -- which can DEFER
  // deletion only for a heap module torn down via terminate(). Refuse any
  // target we cannot pin: a null module (independent/standalone host
  // function) has nothing to pin, and a stack/member (embedder-constructed)
  // module cannot be deferred. Both would leave the worker dereferencing
  // storage a caller may destroy mid-flight (UAF). Reject up front (invalid
  // Async), mirroring the closing refusal above; invoke such targets
  // synchronously.
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
  // const auto &FuncType = FuncInst->getFuncType();
  // const auto PTypes = FuncType.getParamList();
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
  std::vector<ValVariant> CoreWASMArgs =
      convValsToCoreWASM(Params, ParamTypes, ReallocFuncInst, MemInst);

  // Call runFunction.
  auto *CoreFuncInst = FuncInst->getLowerFunction();
  assuming(CoreFuncInst);
  const auto &CoreFuncType = CoreFuncInst->getFuncType();
  // TODO: COMPONENT - check the ABI types between core functype and args.
  EXPECTED_TRY(auto CoreWASMReturns, invoke(CoreFuncInst, CoreWASMArgs,
                                            CoreFuncType.getParamTypes()));

  // Get return values.
  std::vector<ComponentValType> ReturnTypes;
  for (const auto &Type : FuncInst->getFuncType().getResultList()) {
    ReturnTypes.push_back(Type.getValType());
  }
  EXPECTED_TRY(auto Returns,
               convValsToComponent(CoreWASMReturns, ReturnTypes, MemInst));
  assuming(Returns.size() == ReturnTypes.size());
  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
