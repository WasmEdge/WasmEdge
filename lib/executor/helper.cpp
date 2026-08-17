// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/spdlog.h"
#include "runtime/storemgr.h"
#include "system/fault.h"
#include "system/stacktrace.h"

#include <cstdint>
#include <shared_mutex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

Executor::SavedThreadLocal::SavedThreadLocal(
    Executor &Ex, Runtime::StackManager &StackMgr,
    [[maybe_unused]] const Runtime::Instance::FunctionInstance &Func) noexcept {
  // Prepare the execution context.
  SavedThis = This;
  This = &Ex;

  SavedExecutionContext = ExecutionContext;
  ExecutionContext.StopToken = &Ex.StopToken;
  ExecutionContext.PendingExnTagAddr =
      reinterpret_cast<void *const *>(&PendingExn.TagInst);
  // Thread this compiled thread's stable shadow-root anchor into the ExecCtx so
  // generated code can publish spilled managed refs. Rebound on every compiled
  // entry; nullptr if the thread has no registered stack (codegen skips the
  // push in that case).
  ExecutionContext.ShadowHead = Ex.getController().currentShadowHead();
  // Thread the controller's stop flag so generated code can poll it inline at
  // loop back-edges and call kGCSafepoint to park when a collection is stopping
  // the world. Stable for the controller's lifetime.
  ExecutionContext.GCStopFlag = Ex.getController().stopFlagPtr();
  if (Ex.Stat) {
    ExecutionContext.InstrCount = &Ex.Stat->getInstrCountRef();
    ExecutionContext.CostTable = Ex.Stat->getCostTable().data();
    ExecutionContext.Gas = &Ex.Stat->getTotalCostRef();
    ExecutionContext.GasLimit = Ex.Stat->getCostLimit();
  }

  SavedCurrentStack = CurrentStack;
  CurrentStack = &StackMgr;
}

Executor::SavedThreadLocal::~SavedThreadLocal() noexcept {
  CurrentStack = SavedCurrentStack;
  ExecutionContext = SavedExecutionContext;
  This = SavedThis;
}

Expect<AST::InstrView::iterator> Executor::enterFunction(
    Runtime::StackManager &StackMgr,
    const Runtime::Instance::FunctionInstance &Func,
    const AST::InstrView::iterator RetIt, bool IsTailCall, bool IsNativeEntry,
    const Runtime::Instance::ModuleInstance *CallerModInst) {
  // RetIt: the return position when the entered function returns.

  if (unlikely(getController().stopRequested())) {
    getController().gcSafepoint();
  }

  // Check whether interruption occurred.
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }

  // Get the function type for the parameter and return counts.
  const auto &FuncType = Func.getFuncType();
  const uint32_t ArgsN = static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t RetsN =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  // For the exception handler, remove the inactive handlers caused by the
  // branches.
  const auto Instrs = Func.getInstrs();
  if (likely(RetIt) && RetIt != Instrs.begin()) {
    StackMgr.removeInactiveHandler(RetIt - 1);
  }

  if (Func.isHostFunction()) {
    // Host function case: Push args and call function.
    auto &HostFunc = Func.getHostFunc();

    // Finalize the host module on its first host-function invocation, after
    // which adding host instances to it is rejected.
    if (const auto *HostModInst = Func.getModule()) {
      HostModInst->finalizeInstantiation();
    }

    // Generate CallingFrame from current frame.
    // The module instance will be nullptr if current frame is a dummy frame.
    // For this case, use the module instance of this host function.
    const auto *ModInst = CallerModInst;
    if (ModInst == nullptr) {
      ModInst = StackMgr.getModule();
    }
    if (ModInst == nullptr) {
      ModInst = Func.getModule();
    }
    Runtime::CallingFrame CallFrame(this, ModInst);

    // Push frame.
    StackMgr.pushFrame(Func.getModule(), // Module instance
                       RetIt,            // Return PC
                       ArgsN,            // Only args, no locals in stack
                       RetsN,            // Returns num
                       IsTailCall,       // For tail-call
                       IsNativeEntry     // For native entry
    );

    // Do the statistics if the statistics turned on.
    if (Stat) {
      // Check host function cost.
      if (unlikely(!Stat->addCost(HostFunc.getCost()))) {
        spdlog::error(ErrCode::Value::CostLimitExceeded);
        return Unexpect(ErrCode::Value::CostLimitExceeded);
      }
      // Start recording time of running host function.
      Stat->stopRecordWasm();
      Stat->startRecordHost();
    }

    // Keep args on the (GC-rooted) value stack, not a detached buffer, so
    // managed refs survive a collection during the call. cleanNumericVal
    // mutates value-stack slots and so must run as Running (before the native
    // scope below): a coordinator scanning a NativeRunning thread's stacks in
    // place must never race a stack write.
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    for (uint32_t I = 0; I < ArgsN; I++) {
      // For the number type cases of the arguments, the unused bits should be
      // erased due to the security issue.
      cleanNumericVal(Args[I], FuncType.getParamTypes()[I]);
    }
    std::vector<ValVariant> Rets(RetsN);
    // Boundary roots: pin any managed reference the host produces into the
    // detached Rets buffer as a host root BEFORE this call's native scope dtor
    // may park at a re-entry safe point -- Rets is on no value stack, so a
    // collection during that park would not otherwise see them. Released (RAII)
    // after pushSpan transfers ownership to the GC-scanned value stack.
    GC::BoundaryRoots RetRoots(getAllocator());
    // Mark this thread NativeRunning for the duration of ALL
    // externally-supplied callbacks -- the pre/post hooks as well as run(): a
    // GC coordinator that starts a setup handshake while we are in native code
    // must not wait for an ack we cannot deliver (native code reaches no safe
    // point), and a blocking hook must not hang it; instead it scans our stable
    // value stacks in place. The scope's dtor restores Running and performs a
    // re-entry safe-point poll, so a handshake that began during the call is
    // acknowledged before guest execution resumes. The surrounding stack
    // mutations (cleanNumericVal above, pushSpan/popFrame below) stay outside
    // as Running.
    auto Ret = [&] {
      GC::Controller::NativeScope Native(getController());
      // Call pre-host-function
      HostFuncHelper.invokePreHostFunc();
      auto R = HostFunc.run(CallFrame, Args, Rets);
      // Pin managed return refs while still NativeRunning and BEFORE the post
      // hook. invokePostHostFunc() runs arbitrary user native code that may
      // block or reenter the runtime, giving a concurrent collection a window
      // in which a just-returned reference lives only in the unscanned Rets
      // buffer (Rets is on no value stack, and this thread is NativeRunning, so
      // the collector scans only its registered guest stacks). Rooting the
      // returns first closes that window; the post hook and the scope dtor's
      // re-entry safe point then both run with every managed return already
      // published as a host root.
      if (R) {
        const auto &RTypes = FuncType.getReturnTypes();
        for (uint32_t I = 0; I < RetsN; ++I) {
          if (RTypes[I].isRefType()) {
            RetRoots.pin(Rets[I].get<RefVariant>());
          }
        }
      }
      // Call post-host-function
      HostFuncHelper.invokePostHostFunc();
      return R;
    }();

    // Do the statistics if the statistics turned on.
    if (Stat) {
      // Stop recording time of running host function.
      Stat->stopRecordHost();
      Stat->startRecordWasm();
    }

    // Check the host function execution status.
    if (!Ret) {
      if (Ret.error() == ErrCode::Value::HostFuncError ||
          Ret.error().getCategory() != ErrCategory::WASM) {
        spdlog::error(Ret.error());
      }
      return Unexpect(Ret);
    }

    // Push returns back to the stack.
    StackMgr.pushSpan(Rets);

    // For host function case, the continuation will be the continuation from
    // the popped frame.
    return StackMgr.popFrame();
  } else if (Func.isCompiledFunction()) {
    // Compiled function case: Execute the function and jump to the
    // continuation.

    // R7-M3 backstop: refuse native code that was not compiled with GC support
    // while this executor runs a collector. Such code polls no safepoint (a
    // stop-the-world would wait forever on it) and spills no shadow roots (a
    // collection that did proceed would miss its live references). The
    // instantiate gate normally deopts these modules to the interpreter, but it
    // only sees modules THIS executor instantiated: registerModule and a direct
    // Executor::invoke can both present function instances that a GC-off
    // executor bound to compiled code. Refuse rather than deopt -- a compiled
    // FunctionInstance carries no instruction body to fall back to. Checked
    // before pushFrame so the refusal leaves the stack untouched.
    if (unlikely(GCEnabled)) {
      const auto *ModInst = Func.getModule();
      if (unlikely(ModInst != nullptr && !ModInst->isGCCompiled())) {
        spdlog::error(ErrCode::Value::IllegalGrammar);
        spdlog::error("    Calling compiled function of a module built without "
                      "GC support while the GC proposal is enabled. Instantiate "
                      "the module with this executor, or use a configuration "
                      "matching the one it was compiled with.");
        return Unexpect(ErrCode::Value::IllegalGrammar);
      }
    }

    // Push frame.
    StackMgr.pushFrame(Func.getModule(), // Module instance
                       RetIt,            // Return PC
                       ArgsN,            // Only args, no locals in stack
                       RetsN,            // Returns num
                       IsTailCall,       // For tail-call
                       IsNativeEntry     // For native entry
    );

    // Prepare arguments. Keep them on the (GC-rooted) value stack, not a
    // detached buffer, so managed refs survive a collection during the call.
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);
    SavedThreadLocal Saved(*this, StackMgr, Func);

    ErrCode Err;
    try {
      // Get symbol and execute the function.
      Fault FaultHandler;
      // R7-M2: capture the entry's mutator state at this compiled boundary so
      // the abnormal-fault recovery below can undo any NativeScope transition
      // the longjmp skips (which would otherwise strand the entry in
      // NativeRunning -- remotely scannable). Only meaningful when this thread
      // holds a registered stack (else there is no entry). volatile: it is read
      // after the setjmp returns non-zero.
      volatile GC::Controller::MutatorState BoundaryState =
          GC::Controller::MutatorState::Running;
      // R7-M2: arm signal-safe shadow-head truncation. On an abnormal fault the
      // longjmp below skips generated code's shadow-frame pops; reset the head
      // to its value at this boundary so no scanner walks an abandoned compiled
      // frame. The head cell's Head is a lock-free atomic<pointer>,
      // layout-compatible with atomic<void*>; pass it type-erased to keep the
      // system/ layer free of a gc/ dependency. Null head (no registered shadow
      // stack) leaves truncation disabled.
      if (ExecutionContext.ShadowHead != nullptr) {
        BoundaryState = getController().currentMutatorState();
        auto *Cell = static_cast<GC::Controller::ShadowHead *>(
            ExecutionContext.ShadowHead);
        static_assert(
            sizeof(std::atomic<void *>) ==
                    sizeof(std::atomic<GC::Controller::ShadowFrame *>) &&
                alignof(std::atomic<void *>) ==
                    alignof(std::atomic<GC::Controller::ShadowFrame *>),
            "atomic pointer layout must match for the type-erased head store");
        FaultHandler.armShadowRestore(
            reinterpret_cast<std::atomic<void *> *>(&Cell->Head),
            Cell->Head.load(std::memory_order_relaxed));
      }
      uint32_t Code = PREPARE_FAULT(FaultHandler);
      if (Code != 0) {
        // R7-M2: undo any NativeScope transition the longjmp skipped -- restore
        // the entry to its boundary state (Running, unless this was a
        // host->guest reentry) so it is not left stranded remotely-scannable.
        // This runs in the normal post-longjmp context (locking/parking legal),
        // distinct from the signal-safe head truncation done in emitFault.
        if (ExecutionContext.ShadowHead != nullptr) {
          getController().restoreStateAfterFault(BoundaryState);
        }
        auto InnerStackTrace = FaultHandler.stacktrace();
        {
          std::array<void *, 256> Buffer;
          auto OuterStackTrace = stackTrace(Buffer);
          while (!OuterStackTrace.empty() && !InnerStackTrace.empty() &&
                 InnerStackTrace[InnerStackTrace.size() - 1] ==
                     OuterStackTrace[OuterStackTrace.size() - 1]) {
            InnerStackTrace = InnerStackTrace.first(InnerStackTrace.size() - 1);
            OuterStackTrace = OuterStackTrace.first(OuterStackTrace.size() - 1);
          }
        }
        auto LiveModules = collectLiveModules(StackMgr);
        StackTraceSize =
            compiledStackTrace(LiveModules, InnerStackTrace, StackTrace).size();
        Err = ErrCode(static_cast<ErrCategory>(Code >> 24), Code);
      } else {
        auto &Wrapper = FuncType.getSymbol();
        Wrapper(
            &const_cast<Runtime::Instance::ModuleInstance *>(Func.getModule())
                 ->ModCtx,
            &ExecutionContext, Func.getSymbol().get(), Args.data(),
            Rets.data());
      }
    } catch (const ErrCode &E) {
      Err = E;
    }
    if (unlikely(Err)) {
      if (Err != ErrCode::Value::Terminated) {
        spdlog::error(Err);
      }
      StackTraceSize +=
          interpreterStackTrace(
              StackMgr,
              Span<StackTraceEntry>{StackTrace}.subspan(StackTraceSize))
              .size();
      return Unexpect(Err);
    }

    if (unlikely(PendingExn.TagInst != nullptr)) {
      // The exception escapes this frame: discard it, then hand off to the
      // native caller or continue the handler walk in the interpreter caller.
      const bool FromNative = StackMgr.isTopFrameNativeEntry();
      // Push the dummy results for popping the frame, then drop them because
      // the escaping exception produces no results.
      for (uint32_t I = 0; I < RetsN; ++I) {
        StackMgr.push(Rets[I]);
      }
      AST::InstrView::iterator ResumePC = StackMgr.popFrame();
      StackMgr.eraseValueStack(RetsN, 0);
      if (FromNative) {
        return Unexpect(ErrCode::Value::PendingException);
      }
      auto &TagInst = *PendingExn.TagInst;
      const auto *ExnInst = PendingExn.Inst;
      StackMgr.pushSpan(PendingExn.getPayload());
      // Cleared only after the payload is back on the (rooted) value stack, so
      // its managed refs are never simultaneously off the stack and off the aux
      // roots.
      PendingExn.clear(getController());
      EXPECTED_TRY(throwException(StackMgr, TagInst, ResumePC, ExnInst));
      return ResumePC + 1;
    }

    // Push returns back to the stack.
    for (uint32_t I = 0; I < Rets.size(); ++I) {
      StackMgr.push(Rets[I]);
    }

    // For compiled function case, the continuation will be the continuation
    // from the popped frame.
    return StackMgr.popFrame();
  } else {
    // WASM interpreter case: Jump to the start of the function body.

    // Push local variables into the stack.
    for (auto &Def : Func.getLocals()) {
      if (Def.second.isRefType() && !Def.second.isAbsHeapType()) {
        // For non-abstract heap types (concrete type indices), convert the
        // null ref to the abstract heap type so that ref.cast/ref.test won't
        // dereference a null pointer when checking the type.
        const auto &CompType = Func.getModule()
                                   ->unsafeGetType(Def.second.getTypeIndex())
                                   ->getCompositeType();
        auto BotTypeCode =
            CompType.isFunc() ? TypeCode::NullFuncRef : TypeCode::NullRef;
        RefVariant InitVal(ValType(TypeCode::RefNull, BotTypeCode));
        for (uint32_t I = 0; I < Def.first; I++) {
          StackMgr.push(InitVal);
        }
      } else {
        for (uint32_t I = 0; I < Def.first; I++) {
          StackMgr.push(ValueFromType(Def.second));
        }
      }
    }

    // Push frame.
    // The PC must -1 here because in the interpreter mode execution, the PC
    // will increase after the callee returns.
    StackMgr.pushFrame(Func.getModule(),           // Module instance
                       RetIt - 1,                  // Return PC
                       ArgsN + Func.getLocalNum(), // Arguments num + local num
                       RetsN,                      // Returns num
                       IsTailCall,                 // For tail-call
                       IsNativeEntry               // For native entry
    );

    // For the WASM interpreter case, the continuation will be the start of the
    // function body.
    return Instrs.begin();
  }
}

std::vector<const Runtime::Instance::ModuleInstance *>
Executor::collectLiveModules(
    const Runtime::StackManager &StackMgr) const noexcept {
  std::vector<const Runtime::Instance::ModuleInstance *> Modules;
  std::unordered_set<const Runtime::Instance::ModuleInstance *> Seen;
  auto AddModule = [&](const Runtime::Instance::ModuleInstance *M) {
    if (M != nullptr && Seen.insert(M).second) {
      Modules.push_back(M);
    }
  };

  for (const auto &Frame : StackMgr.getFramesSpan()) {
    AddModule(Frame.Module);
  }

  std::unordered_set<Runtime::StoreManager *> Stores;
  auto GatherStores = [&](const Runtime::Instance::ModuleInstance *M) {
    if (M == nullptr) {
      return;
    }
    std::shared_lock Lock(M->Mutex);
    for (const auto &Entry : M->LinkedStore) {
      Stores.insert(Entry.first.first);
    }
  };

  const size_t SeedCount = Modules.size();
  for (size_t I = 0; I < SeedCount; ++I) {
    GatherStores(Modules[I]);
    for (const auto *Func : Modules[I]->getFunctionInstances()) {
      if (Func != nullptr) {
        GatherStores(Func->getModule());
      }
    }
  }

  for (auto *Store : Stores) {
    Store->getModuleList([&AddModule](const auto &NamedMod) {
      for (const auto &Entry : NamedMod) {
        AddModule(Entry.second);
      }
    });
  }
  return Modules;
}

Expect<void>
Executor::branchToLabel(Runtime::StackManager &StackMgr,
                        const AST::Instruction::JumpDescriptor &JumpDesc,
                        AST::InstrView::iterator &PC) noexcept {
  // Check the stop token.
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }

  StackMgr.eraseValueStack(JumpDesc.StackEraseBegin, JumpDesc.StackEraseEnd);
  // PC needs -1 here because the PC will increase in the next iteration.
  PC += (JumpDesc.PCOffset - 1);
  // A branch leaves the innermost blocks without running their `end`, so the
  // handlers it strands are the top of the handler stack right now. Drop them
  // here: once a later try_table is pushed on top they are indistinguishable
  // from active handlers, and their stale VPos would invert the erase range in
  // popTopHandler. PC + 1 is the instruction being branched to.
  StackMgr.removeInactiveHandler(PC + 1);
  return {};
}

Expect<void> Executor::throwException(
    Runtime::StackManager &StackMgr, Runtime::Instance::TagInstance &TagInst,
    AST::InstrView::iterator &PC,
    const Runtime::Instance::ExceptionInstance *ExnInst) noexcept {
  StackMgr.removeInactiveHandler(PC);
  auto AssocValSize = TagInst.getTagType().getAssocValSize();
  while (true) {
    // Pop the top handler.
    auto Handler = StackMgr.popTopHandler(AssocValSize);
    if (!Handler.has_value()) {
      break;
    }
    // Checking through the catch clause.
    for (const auto &C : Handler->CatchClause) {
      if (!C.IsAll &&
          getTagInstByIdx(StackMgr.getModule(), C.TagIndex) != &TagInst) {
        // Specific-tag clauses require tag-address equivalence; skip the
        // ones that do not match.
        continue;
      }
      if (C.IsRef) {
        // Allocate the exception instance lazily on the first catch_ref;
        // reuse the one passed in by throw_ref to preserve exnref identity.
        const Runtime::Instance::ExceptionInstance *Inst = ExnInst;
        if (Inst == nullptr) {
          // Copy the top AssocValSize payload without removing it: catch_ref
          // leaves the payload in place and pushes the exnref above it. Copying
          // via getTopSpan (not pop-then-push) also keeps the managed refs
          // rooted, with no window for a concurrent collection to miss them.
          auto Payload = StackMgr.getTopSpan(AssocValSize);
          std::vector<ValVariant> Vec(Payload.begin(), Payload.end());
          auto *ModInst = const_cast<Runtime::Instance::ModuleInstance *>(
              StackMgr.getModule());
          Inst =
              ModInst->newException(getAllocator(), &TagInst, std::move(Vec));
        }
        if (C.IsAll) {
          StackMgr.eraseValueStack(AssocValSize, 0);
        }
        StackMgr.push(
            RefVariant(ValType(TypeCode::Ref, TypeCode::ExnRef), Inst));
      } else if (C.IsAll) {
        StackMgr.eraseValueStack(AssocValSize, 0);
      }
      // When an exception is caught, move the PC to the try block and branch to
      // the label.

      PC = Handler->Try;
      return branchToLabel(StackMgr, C.Jump, PC);
    }
  }
  if (StackMgr.isTopFrameNativeEntry()) {
    // Stopped at a frame entered from the native code: record the exception
    // as pending and restore the stack; the native caller continues it.
    PendingExn.TagInst = &TagInst;
    PendingExn.Inst = ExnInst;
    // Copied while the payload is still on the rooted value stack, and rooted
    // as this thread's aux roots from the moment it lands -- the erase below
    // therefore never leaves the managed refs unrooted.
    PendingExn.setPayload(getController(), StackMgr.getTopSpan(AssocValSize));
    // Push the dummy results for popping the frame, then drop them because
    // the escaping exception produces no results.
    const uint32_t Arity = StackMgr.getFramesSpan().back().Arity;
    for (uint32_t I = 0; I < Arity; ++I) {
      StackMgr.push(ValVariant());
    }
    StackMgr.popFrame();
    StackMgr.eraseValueStack(Arity, 0);
    return Unexpect(ErrCode::Value::PendingException);
  }
  spdlog::error(ErrCode::Value::UncaughtException);
  return Unexpect(ErrCode::Value::UncaughtException);
}

Expect<void>
Executor::checkOffsetOverflow(const Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr, const uint64_t Val,
                              const uint64_t Size) const noexcept {
  // This function simply checks that the calculated offset fits in 64 bits.
  uint64_t StartOffset;
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  if (std::numeric_limits<uint64_t>::max() - Instr.getMemoryOffset() < Val) {
    StartOffset = Instr.getMemoryOffset() + Val;
#else
  if (unlikely(
          __builtin_add_overflow(Instr.getMemoryOffset(), Val, &StartOffset))) {
#endif
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoBoundary(StartOffset, Size, MemInst.getSize(), true));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return {};
}

const AST::SubType *
Executor::getDefTypeByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                          const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetType(Idx);
}

const WasmEdge::AST::CompositeType &Executor::getCompositeTypeByIdx(
    const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t Idx) const noexcept {
  auto *DefType = getDefTypeByIdx(ModInst, Idx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  return CompType;
}

const ValType &Executor::getStructStorageTypeByIdx(
    const Runtime::Instance::ModuleInstance *ModInst, const uint32_t Idx,
    const uint32_t Off) const noexcept {
  const auto &CompType = getCompositeTypeByIdx(ModInst, Idx);
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) > Off);
  return CompType.getFieldTypes()[Off].getStorageType();
}

const ValType &Executor::getArrayStorageTypeByIdx(
    const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t Idx) const noexcept {
  const auto &CompType = getCompositeTypeByIdx(ModInst, Idx);
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  return CompType.getFieldTypes()[0].getStorageType();
}

Runtime::Instance::FunctionInstance *
Executor::getFuncInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                           const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetFunction(Idx);
}

Runtime::Instance::TableInstance *
Executor::getTabInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                          const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetTable(Idx);
}

Runtime::Instance::MemoryInstance *
Executor::getMemInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                          const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetMemory(Idx);
}

Runtime::Instance::TagInstance *
Executor::getTagInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                          const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetTag(Idx);
}

Runtime::Instance::GlobalInstance *
Executor::getGlobInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                           const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetGlobal(Idx);
}

Runtime::Instance::ElementInstance *
Executor::getElemInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                           const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetElem(Idx);
}

Runtime::Instance::DataInstance *
Executor::getDataInstByIdx(const Runtime::Instance::ModuleInstance *ModInst,
                           const uint32_t Idx) const {
  // When the top frame is a dummy frame, the instance cannot be found.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetData(Idx);
}

TypeCode
Executor::toBottomType(const Runtime::Instance::ModuleInstance *ModInst,
                       const ValType &Type) const {
  if (Type.isRefType()) {
    if (Type.isAbsHeapType()) {
      switch (Type.getHeapTypeCode()) {
      case TypeCode::NullFuncRef:
      case TypeCode::FuncRef:
        return TypeCode::NullFuncRef;
      case TypeCode::NullExternRef:
      case TypeCode::ExternRef:
        return TypeCode::NullExternRef;
      case TypeCode::NullRef:
      case TypeCode::AnyRef:
      case TypeCode::EqRef:
      case TypeCode::I31Ref:
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
        return TypeCode::NullRef;
      case TypeCode::NullExnRef:
      case TypeCode::ExnRef:
        return TypeCode::NullExnRef;
      default:
        assumingUnreachable();
      }
    } else {
      const auto &CompType =
          ModInst->unsafeGetType(Type.getTypeIndex())->getCompositeType();
      if (CompType.isFunc()) {
        return TypeCode::NullFuncRef;
      } else {
        return TypeCode::NullRef;
      }
    }
  } else {
    return Type.getCode();
  }
}

void Executor::cleanNumericVal(ValVariant &Val,
                               const ValType &Type) const noexcept {
  if (Type.isNumType()) {
    switch (Type.getCode()) {
    case TypeCode::I32: {
      uint32_t V = Val.get<uint32_t>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<uint32_t>(V);
      break;
    }
    case TypeCode::F32: {
      float V = Val.get<float>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<float>(V);
      break;
    }
    case TypeCode::I64: {
      uint64_t V = Val.get<uint64_t>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<uint64_t>(V);
      break;
    }
    case TypeCode::F64: {
      double V = Val.get<double>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<double>(V);
      break;
    }
    default:
      break;
    }
  }
}

ValVariant Executor::packVal(const ValType &Type,
                             const ValVariant &Val) const noexcept {
  if (Type.isPackType()) {
    switch (Type.getCode()) {
    case TypeCode::I8:
      if constexpr (Endian::native == Endian::little) {
        return ValVariant(Val.get<uint32_t>() & 0xFFU);
      } else {
        return ValVariant(Val.get<uint32_t>() << 24);
      }
    case TypeCode::I16:
      if constexpr (Endian::native == Endian::little) {
        return ValVariant(Val.get<uint32_t>() & 0xFFFFU);
      } else {
        return ValVariant(Val.get<uint32_t>() << 16);
      }
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

std::vector<ValVariant>
Executor::packVals(const ValType &Type,
                   std::vector<ValVariant> &&Vals) const noexcept {
  for (uint32_t I = 0; I < Vals.size(); I++) {
    Vals[I] = packVal(Type, Vals[I]);
  }
  return std::move(Vals);
}

ValVariant Executor::unpackVal(const ValType &Type, const ValVariant &Val,
                               bool IsSigned) const noexcept {
  if (Type.isPackType()) {
    uint32_t Num = Val.get<uint32_t>();
    switch (Type.getCode()) {
    case TypeCode::I8:
      if constexpr (Endian::native == Endian::big) {
        Num >>= 24;
      }
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int8_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint8_t>(Num));
      }
    case TypeCode::I16:
      if constexpr (Endian::native == Endian::big) {
        Num >>= 16;
      }
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int16_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint16_t>(Num));
      }
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

} // namespace Executor
} // namespace WasmEdge
