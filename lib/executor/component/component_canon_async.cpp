// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_canon_async.cpp - async canonical built-in functions ----===//
//
// Runtime behavior of the async canonical built-ins over the task runtime.
//
//===----------------------------------------------------------------------===//

#include "executor/component/canonical_abi.h"
#include "runtime/component/taskmgr.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include <algorithm>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

namespace {

// A trap whose code says it all.
Expect<void> trap(ErrCode::Value Code) noexcept {
  spdlog::error(Code);
  return Unexpect(Code);
}

// A trap with the detail the code does not carry.
Expect<void> trapMsg(ErrCode::Value Code, std::string_view Msg) noexcept {
  spdlog::error(Code);
  spdlog::error("    {}"sv, Msg);
  return Unexpect(Code);
}

Expect<void> trapCannotLeave() noexcept {
  return trap(ErrCode::Value::ComponentCannotLeave);
}

Expect<void> trapUnknownHandle(uint32_t Idx) noexcept {
  return trapMsg(ErrCode::Value::ComponentHandleUnknown,
                 fmt::format("handle index {}"sv, Idx));
}

Expect<void> trapCannotBlock() noexcept {
  return trap(ErrCode::Value::ComponentCannotBlockSync);
}

// True when the element type permits same-instance transfers.
bool noneOrNumber(PrimValType P) noexcept {
  switch (P) {
  case PrimValType::S8:
  case PrimValType::U8:
  case PrimValType::S16:
  case PrimValType::U16:
  case PrimValType::S32:
  case PrimValType::U32:
  case PrimValType::S64:
  case PrimValType::U64:
  case PrimValType::F32:
  case PrimValType::F64:
    return true;
  default:
    return false;
  }
}

bool noneOrNumber(const Runtime::Instance::ComponentInstance *Inst,
                  const std::optional<ComponentValType> &T) noexcept {
  if (!T.has_value()) {
    return true;
  }
  if (!T->isPrimValType()) {
    const AST::Component::DefType *Def = nullptr;
    if (Inst != nullptr) {
      if (const auto Found = Inst->getType(T->getTypeIndex())) {
        Def = *Found;
      }
    }
    if (Def == nullptr || !Def->isDefValType() ||
        !Def->getDefValType().isPrimValType()) {
      return false;
    }
    return noneOrNumber(Def->getDefValType().getPrimValType());
  }
  return noneOrNumber(T->getPrimValType());
}

// Build the lift/lower context for one side of a copy.
LiftLowerContext
bufferCtx(ComponentExecutor *Exec,
          const Runtime::Instance::Component::TransmitBuffer &B) noexcept {
  const bool Aliased = B.ElemInst != nullptr && B.ElemInst != B.Opts.Inst;
  LiftLowerContext Ctx(B.Opts, Exec, Aliased ? B.ElemInst : nullptr);
  Ctx.setCrossComponent(true);
  return Ctx;
}

// Validate a guest buffer against its memory.
Expect<void>
checkBuffer(ComponentExecutor *Exec,
            const Runtime::Instance::Component::TransmitBuffer &B) noexcept {
  if (B.Length > LiftLowerContext::MaxCanonByteLength) {
    return trap(ErrCode::Value::ComponentStreamOpTooBig);
  }
  if (B.Elem.has_value() && B.Length > 0) {
    auto Ctx = bufferCtx(Exec, B);
    EXPECTED_TRY(auto Align, Ctx.alignment(*B.Elem));
    EXPECTED_TRY(auto Size, Ctx.elemSize(*B.Elem));
    if (Align != 0 && (B.Ptr % Align) != 0) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "buffer pointer is not aligned"sv);
    }
    if (B.Opts.Mem == nullptr ||
        !B.Opts.Mem->checkAccessBound(B.Ptr, B.Length * Size)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "buffer region out of bounds of memory"sv);
    }
  }
  return {};
}

// Load N elements of Src from its progress on, out of guest or host memory.
Expect<std::vector<ComponentValVariant>>
loadElements(ComponentExecutor *Exec,
             const Runtime::Instance::Component::TransmitBuffer &Src,
             uint32_t N) noexcept {
  std::vector<ComponentValVariant> Vals;
  Vals.reserve(N);
  if (Src.isHost()) {
    const uint32_t From = Src.Host->Base + Src.Progress;
    for (uint32_t I = 0; I < N; ++I) {
      if (Src.isByteElem()) {
        Vals.emplace_back(Src.Host->Bytes[From + I]);
      } else {
        Vals.push_back(Src.Host->Vals[From + I]);
      }
    }
    return Vals;
  }
  auto Ctx = bufferCtx(Exec, Src);
  EXPECTED_TRY(auto Size, Ctx.elemSize(*Src.Elem));
  for (uint32_t I = 0; I < N; ++I) {
    EXPECTED_TRY(auto V,
                 Ctx.load(Src.Ptr + (Src.Progress + I) * Size, *Src.Elem));
    Vals.push_back(std::move(V));
  }
  return Vals;
}

// Store loaded elements into Dst from its progress on.
Expect<void> storeElements(ComponentExecutor *Exec,
                           Runtime::Instance::Component::TransmitBuffer &Dst,
                           Span<const ComponentValVariant> Vals) noexcept {
  if (Dst.isHost()) {
    const uint32_t To = Dst.Host->Base + Dst.Progress;
    for (uint32_t I = 0; I < Vals.size(); ++I) {
      if (Dst.isByteElem()) {
        Dst.Host->Bytes[To + I] = std::get<uint8_t>(Vals[I]);
      } else {
        Dst.Host->Vals[To + I] = Vals[I];
      }
    }
    return {};
  }
  auto Ctx = bufferCtx(Exec, Dst);
  EXPECTED_TRY(auto Size, Ctx.elemSize(*Dst.Elem));
  for (uint32_t I = 0; I < Vals.size(); ++I) {
    EXPECTED_TRY(
        Ctx.store(Vals[I], *Dst.Elem, Dst.Ptr + (Dst.Progress + I) * Size));
  }
  return {};
}

// Move N elements from Src into Dst, advancing both progress counters.
Expect<void> copyElements(ComponentExecutor *Exec,
                          Runtime::Instance::Component::TransmitBuffer &Dst,
                          Runtime::Instance::Component::TransmitBuffer &Src,
                          uint32_t N) noexcept {
  if (!Src.Elem.has_value() || !Dst.Elem.has_value()) {
    Src.Progress += N;
    Dst.Progress += N;
    return {};
  }
  // Bytes between guest memory and a host buffer move without lifting.
  if (Src.isByteElem() && Dst.isByteElem() && Src.isHost() != Dst.isHost()) {
    if (Src.isHost()) {
      auto Out = Dst.Opts.Mem->getSpan<uint8_t>(Dst.Ptr + Dst.Progress, N);
      std::copy_n(Src.Host->Bytes.begin() + Src.Host->Base + Src.Progress, N,
                  Out.begin());
    } else {
      auto In = Src.Opts.Mem->getSpan<const uint8_t>(Src.Ptr + Src.Progress, N);
      std::copy(In.begin(), In.end(),
                Dst.Host->Bytes.begin() + Dst.Host->Base + Dst.Progress);
    }
    Src.Progress += N;
    Dst.Progress += N;
    return {};
  }
  // Read every element before writing, so same-memory copies act as memmove.
  EXPECTED_TRY(auto Vals, loadElements(Exec, Src, N));
  if (Src.isHost() && !Dst.isHost()) {
    // Host-written values enter guest memory here; storing reads them
    // unchecked.
    auto Ctx = bufferCtx(Exec, Dst);
    for (const auto &V : Vals) {
      EXPECTED_TRY(Ctx.checkValue(V, *Dst.Elem));
    }
  }
  EXPECTED_TRY(storeElements(Exec, Dst, Vals));
  Src.Progress += N;
  Dst.Progress += N;
  return {};
}

// Hand N elements of Src to the sink of Dst; false when the sink refused.
Expect<bool> drainToSink(ComponentExecutor *Exec,
                         Runtime::Instance::Component::TransmitBuffer &Dst,
                         Runtime::Instance::Component::TransmitBuffer &Src,
                         uint32_t N) noexcept {
  if (N == 0 || !Src.Elem.has_value()) {
    Src.Progress += N;
    return true;
  }
  bool Ok = true;
  if (Dst.Host->ByteSink && Src.isByteElem()) {
    if (Src.isHost()) {
      Ok = Dst.Host->ByteSink(Span<const uint8_t>(
          Src.Host->Bytes.data() + Src.Host->Base + Src.Progress, N));
    } else {
      Ok = Dst.Host->ByteSink(
          Src.Opts.Mem->getSpan<const uint8_t>(Src.Ptr + Src.Progress, N));
    }
  } else {
    EXPECTED_TRY(auto Vals, loadElements(Exec, Src, N));
    if (Dst.Host->ValSink) {
      Ok = Dst.Host->ValSink(Vals);
    } else {
      std::vector<uint8_t> Bytes;
      Bytes.reserve(Vals.size());
      for (const auto &V : Vals) {
        Bytes.push_back(std::get<uint8_t>(V));
      }
      Ok = Dst.Host->ByteSink(Bytes);
    }
  }
  if (Ok) {
    Src.Progress += N;
  }
  return Ok;
}

// A refusing sink ends the stream for both sides.
void failSink(Runtime::Instance::Component::TransmitEnd &Sink,
              Runtime::Instance::Component::TransmitEnd &Writer,
              Runtime::Instance::Component::TransmitState &Shared) noexcept {
  Shared.Dropped = true;
  Shared.PendingDone = false;
  Shared.PendingEnd = nullptr;
  Writer.queueCopyEvent(Runtime::Instance::Component::TransmitResult::Dropped,
                        false);
  Sink.queueCopyEvent(Runtime::Instance::Component::TransmitResult::Cancelled,
                      false);
}

} // namespace

} // namespace Component

Expect<void> ComponentExecutor::startCopy(
    Runtime::Instance::Component::TransmitEnd &E) noexcept {
  using Component::copyElements;
  using Component::drainToSink;
  using Component::failSink;
  using Component::noneOrNumber;
  using Component::trap;
  using Component::trapMsg;
  using Runtime::Instance::Component::TransmitResult;
  auto Shared = E.Shared;
  const bool IsFuture = E.isFuture();
  const bool IsWrite =
      E.getKind() ==
          Runtime::Instance::Component::WaitableBase::Kind::StreamWrite ||
      E.getKind() ==
          Runtime::Instance::Component::WaitableBase::Kind::FutureWrite;
  E.Status = Runtime::Instance::Component::TransmitEnd::State::Copying;

  if (Shared->Dropped) {
    // The peer end is gone: the copy resolves immediately as dropped.
    E.queueCopyEvent(TransmitResult::Dropped, false);
    return {};
  }
  if (Shared->PendingEnd == nullptr || Shared->PendingDone) {
    // Park this side; an exhausted uncollected side no longer joins.
    Shared->PendingDone = false;
    Shared->PendingEnd = &E;
    return {};
  }
  auto *Peer = Shared->PendingEnd;
  if (!E.Buffer.isHost() && !Peer->Buffer.isHost() &&
      Peer->Buffer.Opts.Inst == E.Buffer.Opts.Inst &&
      !noneOrNumber(E.Buffer.ElemInst, E.Buffer.Elem)) {
    return trap(ErrCode::Value::ComponentIntraCopy);
  }
  // Src is the writer's buffer, Dst the reader's; E is the current end.
  auto &Src = IsWrite ? E.Buffer : Peer->Buffer;
  auto &Dst = IsWrite ? Peer->Buffer : E.Buffer;

  // A host sink takes every element of the writer and stays parked.
  if (IsWrite && Peer->Buffer.isSink()) {
    EXPECTED_TRY(const bool Ok,
                 drainToSink(this, Dst, Src, Src.getRemaining()));
    if (!Ok) {
      failSink(*Peer, E, *Shared);
      return {};
    }
    Peer->Buffer.Progress = 0;
    E.queueCopyEvent(TransmitResult::Completed, false);
    return {};
  }
  if (!IsWrite && E.Buffer.isSink()) {
    // The sink starts against a parked writer: take all of it, then park.
    EXPECTED_TRY(const bool Ok,
                 drainToSink(this, Dst, Src, Src.getRemaining()));
    Shared->PendingDone = false;
    Shared->PendingEnd = nullptr;
    if (!Ok) {
      failSink(E, *Peer, *Shared);
      return {};
    }
    Peer->queueCopyEvent(TransmitResult::Completed, true);
    E.Buffer.Progress = 0;
    Shared->PendingDone = false;
    Shared->PendingEnd = &E;
    return {};
  }

  // Mark the parked side finished; its Completed event waits for collection.
  auto CompletePeer = [&]() {
    Shared->PendingDone = true;
    Peer->queueCopyEvent(TransmitResult::Completed, true);
  };
  // Transfer through the parked buffer until it fills up.
  auto TransferToPeer = [&]() -> Expect<void> {
    if (E.Buffer.getRemaining() > 0) {
      const uint32_t N =
          std::min(E.Buffer.getRemaining(), Peer->Buffer.getRemaining());
      EXPECTED_TRY(copyElements(this, Dst, Src, N));
      Peer->queueCopyEvent(TransmitResult::Completed, true);
      if (Peer->Buffer.getRemaining() == 0) {
        Shared->PendingDone = true;
      }
    }
    E.queueCopyEvent(TransmitResult::Completed, false);
    return {};
  };
  if (IsFuture) {
    // Single-element rendezvous: move the value and clear the pending slot.
    EXPECTED_TRY(copyElements(this, Dst, Src, 1));
    Shared->PendingEnd = nullptr;
    Peer->queueCopyEvent(TransmitResult::Completed, false);
    E.queueCopyEvent(TransmitResult::Completed, false);
  } else if (IsWrite) {
    // Writer active, reader parked.
    if (Peer->Buffer.getRemaining() > 0) {
      EXPECTED_TRY(TransferToPeer());
    } else if (E.Buffer.isZeroLength() && Peer->Buffer.isZeroLength()) {
      // Both zero-length: the writer completes, the reader stays pending.
      E.queueCopyEvent(TransmitResult::Completed, false);
    } else {
      // The reader's buffer is full or zero: complete it and park the writer.
      CompletePeer();
      Shared->PendingEnd = &E;
      Shared->PendingDone = false;
    }
  } else {
    // Reader active, writer parked.
    if (Peer->Buffer.getRemaining() > 0) {
      EXPECTED_TRY(TransferToPeer());
    } else {
      // The pending writer is zero-length: complete it and park the reader.
      CompletePeer();
      Shared->PendingEnd = &E;
      Shared->PendingDone = false;
    }
  }
  return {};
}

namespace Component {

CanonAsyncBuiltinHostFunc::CanonAsyncBuiltinHostFunc(
    ComponentExecutor *CompExec, AsyncBuiltinInfo BuiltinInfo) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(CompExec),
      Info(std::move(BuiltinInfo)) {
  auto &FnType = DefType.getCompositeType().getFuncType();
  if (Info.Code == ComponentCanonOpCode::Task__return) {
    // Params = flatten of the declared result list.
    LiftLowerContext Ctx{Info.Opts};
    std::vector<ValType> Flat;
    bool Indirect = false;
    for (const auto &T : Info.RetTypes) {
      auto Sub = Ctx.flattenType(T);
      if (!Sub) {
        Indirect = true;
        break;
      }
      Flat.insert(Flat.end(), Sub->begin(), Sub->end());
    }
    if (Indirect || Flat.size() > LiftLowerContext::MaxFlatParams) {
      Flat.clear();
      Flat.push_back(ValType(TypeCode::I32));
    }
    for (const auto &P : Flat) {
      FnType.getParamTypes().push_back(P);
    }
    return;
  }
  if (Info.Code == ComponentCanonOpCode::Context__get) {
    FnType = AST::FunctionType({}, {Info.ContextType});
    return;
  }
  if (Info.Code == ComponentCanonOpCode::Context__set) {
    FnType = AST::FunctionType({Info.ContextType}, {});
    return;
  }
  // The address type of the memory option, or of the table of
  // thread.new-indirect.
  ValType Addr = LiftLowerContext{Info.Opts}.getPtrType();
  if (Info.Code == ComponentCanonOpCode::Thread__new_indirect) {
    Addr = ValType(Info.Table != nullptr &&
                           Info.Table->getTableType().getLimit().is64()
                       ? TypeCode::I64
                       : TypeCode::I32);
  }
  const auto [Params, Results] =
      AST::Component::Canonical::getBuiltinCoreFuncType(Info.Code, Addr);
  FnType = AST::FunctionType(Params, Results);
}

Expect<void> CanonAsyncBuiltinHostFunc::run(const Runtime::CallingFrame &,
                                            Span<const ValVariant> Args,
                                            Span<ValVariant> Rets) {
  auto &TaskMgr = Exec->getTaskManager();
  const auto *Inst = Info.Opts.Inst;
  Runtime::Component::Task *CurTask = TaskMgr.getCurrentTask();

  // Only built-ins that never leave the instance stay callable.
  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set:
    break;
  default:
    if (!Inst->isLeaveAllowed()) {
      return trapCannotLeave();
    }
    break;
  }

  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__inc:
    if (!Inst->incBackpressure()) {
      return trap(ErrCode::Value::ComponentBackpressureOverflow);
    }
    return {};
  case ComponentCanonOpCode::Backpressure__dec:
    if (!Inst->decBackpressure()) {
      return trapMsg(ErrCode::Value::ComponentBackpressureOverflow,
                     "backpressure counter underflow"sv);
    }
    return {};
  case ComponentCanonOpCode::Thread__index: {
    auto *Ctx = TaskMgr.getCurrentContext();
    Rets[0] = Ctx != nullptr ? Ctx->Index : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Context__get: {
    auto *Ctx = TaskMgr.getCurrentContext();
    const uint64_t Slot =
        Ctx != nullptr ? Ctx->Storage[Info.ContextIdx & 1] : 0;
    Rets[0] = LiftLowerContext::slotFromBits(Info.ContextType, Slot);
    return {};
  }
  case ComponentCanonOpCode::Context__set: {
    if (auto *Ctx = TaskMgr.getCurrentContext(); Ctx != nullptr) {
      Ctx->Storage[Info.ContextIdx & 1] =
          LiftLowerContext::slotBits(Args[0], Info.ContextType);
    }
    return {};
  }

  case ComponentCanonOpCode::Yield: {
    if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
      // Yielding in a non-blocking context is a no-op.
      Rets[0] = UINT32_C(0);
      return {};
    }
    EXPECTED_TRY(auto Reason,
                 Exec->getTaskManager().taskWait(
                     *CurTask, []() { return true; }, Info.isCancellable()));
    if (Reason == Runtime::Component::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Reason == Runtime::Component::ResumeReason::Cancelled
                  ? UINT32_C(1)
                  : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Task__return: {
    if (CurTask == nullptr || !CurTask->Opts.Async) {
      return trap(ErrCode::Value::ComponentTaskReturnInvalid);
    }
    // The declared result types and lift options must equal the task's.
    const auto &TaskResults = CurTask->FuncType->getResultList();
    bool TypesMatch = TaskResults.size() == Info.RetTypes.size();
    if (TypesMatch) {
      for (size_t I = 0; I < Info.RetTypes.size(); ++I) {
        if (!LiftLowerContext::valTypeEq(Inst, Info.RetTypes[I],
                                         CurTask->Opts.Inst,
                                         TaskResults[I].getValType())) {
          TypesMatch = false;
          break;
        }
      }
    }
    if (!TypesMatch) {
      return trap(ErrCode::Value::ComponentTaskReturnInvalid);
    }
    if ((Info.Opts.Mem != nullptr && Info.Opts.Mem != CurTask->Opts.Mem) ||
        Info.Opts.Enc != CurTask->Opts.Enc) {
      return trap(ErrCode::Value::ComponentTaskReturnInvalid);
    }
    LiftLowerContext Ctx{Info.Opts, Exec};
    EXPECTED_TRY(auto Results, Ctx.liftValues(Args, Info.RetTypes,
                                              LiftLowerContext::MaxFlatParams));
    return Exec->getTaskManager().taskReturn(*CurTask, std::move(Results));
  }
  case ComponentCanonOpCode::Task__cancel: {
    if (CurTask == nullptr || !CurTask->Opts.Async) {
      return trap(ErrCode::Value::ComponentTaskNotCancelled);
    }
    return Exec->getTaskManager().taskCancel(*CurTask);
  }

  case ComponentCanonOpCode::Waitable_set__new:
    Rets[0] = Inst->addWaitableSet();
    return {};

  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    const uint64_t Ptr = LiftLowerContext{Info.Opts}.liftPtr(Args[1]);
    auto *WSet = Inst->getWaitableSet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    EXPECTED_TRY(Exec->drainPostedTransmits());
    Runtime::Instance::Component::AsyncEvent Ev;
    if (Info.Code == ComponentCanonOpCode::Waitable_set__wait) {
      if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
        return trapCannotBlock();
      }
      WSet->NumWaiting += 1;
      auto ReasonOrErr = Exec->getTaskManager().taskWait(
          *CurTask, [WSet]() { return WSet->hasPendingEvent(); },
          Info.isCancellable(),
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      WSet->NumWaiting -= 1;
      Ev = Reason == Runtime::Component::ResumeReason::Cancelled
               ? Runtime::Instance::Component::
                     AsyncEvent{Runtime::Instance::Component::AsyncEventCode::
                                    TaskCancelled,
                                0, 0}
               : WSet->takePendingEvent();
    } else {
      if (CurTask != nullptr && Info.isCancellable() &&
          CurTask->Status == Runtime::Component::Task::State::PendingCancel) {
        CurTask->Status = Runtime::Component::Task::State::CancelDelivered;
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else if (!WSet->hasPendingEvent()) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::None, 0, 0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    if (Info.Opts.Mem == nullptr || !Info.Opts.Mem->checkAccessBound(Ptr, 8)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "event payload out of bounds of memory"sv);
    }
    EXPECTED_TRY(Info.Opts.Mem->storeValue(Ev.P1, Ptr));
    EXPECTED_TRY(Info.Opts.Mem->storeValue(Ev.P2, Ptr + 4));
    Rets[0] = static_cast<uint32_t>(Ev.Code);
    return {};
  }

  case ComponentCanonOpCode::Waitable_set__drop: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    auto *WSet = Inst->getWaitableSet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    if (!WSet->Elems.empty() || WSet->NumWaiting > 0) {
      return trap(ErrCode::Value::ComponentWaitableSetNotEmpty);
    }
    Inst->removeWaitableSet(SetIdx);
    return {};
  }

  case ComponentCanonOpCode::Waitable__join: {
    const uint32_t WIdx = Args[0].get<uint32_t>();
    const uint32_t SetIdx = Args[1].get<uint32_t>();
    auto *W = Inst->getWaitable(WIdx);
    if (W == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(WIdx));
    }
    if (W->HasSyncWaiter) {
      return trapMsg(
          ErrCode::Value::ComponentWaitableInSetSyncUse,
          "waitable cannot be used synchronously while added to a waitable "
          "set"sv);
    }
    if (SetIdx == 0) {
      W->join(nullptr);
    } else {
      auto *WSet = Inst->getWaitableSet(SetIdx);
      if (WSet == nullptr) {
        EXPECTED_TRY(trapUnknownHandle(SetIdx));
      }
      W->join(WSet);
    }
    return {};
  }

  case ComponentCanonOpCode::Subtask__cancel: {
    // A sync subtask.cancel in a task that cannot block traps first.
    if (!Info.isAsync() && (CurTask == nullptr || !Exec->mayBlock(*CurTask))) {
      return trapCannotBlock();
    }
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->getWaitable(Idx);
    if (W == nullptr ||
        W->getKind() !=
            Runtime::Instance::Component::WaitableBase::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<Runtime::Instance::Component::Subtask *>(W);
    if (Sub->isResolveDelivered()) {
      return trapMsg(
          ErrCode::Value::ComponentSubtaskCancelTerminal,
          "`subtask.cancel` called after terminal status delivered"sv);
    }
    if (Sub->CancellationRequested) {
      return trapMsg(
          ErrCode::Value::ComponentSubtaskCancelTerminal,
          "`subtask.cancel` called after terminal status delivered"sv);
    }
    if (Sub->isInWaitableSet() && !Info.isAsync()) {
      return trapMsg(
          ErrCode::Value::ComponentWaitableInSetSyncUse,
          "waitable cannot be used synchronously while added to a waitable "
          "set"sv);
    }
    if (!Sub->isResolved()) {
      Sub->CancellationRequested = true;
      if (Sub->OnCancel) {
        Sub->OnCancel();
      }
      if (!Sub->isResolved()) {
        if (!Info.isAsync()) {
          if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
            return trapCannotBlock();
          }
          W->HasSyncWaiter = true;
          auto ReasonOrErr = Exec->getTaskManager().taskWait(
              *CurTask, [W]() { return W->hasPendingEvent(); },
              /*Cancellable=*/false);
          EXPECTED_TRY(auto Reason, ReasonOrErr);
          if (Reason == Runtime::Component::ResumeReason::Abort) {
            return Unexpect(ErrCode::Value::ComponentAsyncAborted);
          }
          W->HasSyncWaiter = false;
        } else {
          Rets[0] = Runtime::Instance::Component::TransmitBlocked;
          return {};
        }
      }
    }
    if (W->hasPendingEvent()) {
      (void)W->takePendingEvent();
    }
    Sub->deliverResolve();
    Rets[0] = static_cast<uint32_t>(Sub->Status);
    return {};
  }

  case ComponentCanonOpCode::Subtask__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->getWaitable(Idx);
    if (W == nullptr ||
        W->getKind() !=
            Runtime::Instance::Component::WaitableBase::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<Runtime::Instance::Component::Subtask *>(W);
    if (!Sub->isResolveDelivered()) {
      return trap(ErrCode::Value::ComponentSubtaskNotResolved);
    }
    // The subtask can outlive its table slot; leave any waitable set now.
    W->join(nullptr);
    Inst->removeWaitable(Idx);
    return {};
  }

  case ComponentCanonOpCode::Thread__new_indirect: {
    if (Info.Table == nullptr) {
      return trap(ErrCode::Value::ComponentThreadStartInvalid);
    }
    const uint64_t Fi = Info.Table->getTableType().getLimit().is64()
                            ? Args[0].get<uint64_t>()
                            : Args[0].get<uint32_t>();
    const uint32_t C = Args[1].get<uint32_t>();
    EXPECTED_TRY(auto Ref, Info.Table->getRefAddr(Fi).map_error([](auto E) {
      spdlog::error("    thread.new-indirect index out of bounds"sv);
      return E;
    }));
    const auto *Fn = retrieveFuncRef(Ref);
    if (Fn == nullptr) {
      return trapMsg(
          ErrCode::Value::ComponentThreadStartInvalid,
          "the start function index points to an uninitialized function"sv);
    }
    const auto &FnTy = Fn->getFuncType();
    if (FnTy.getParamTypes() != std::vector<ValType>{ValType(TypeCode::I32)} ||
        !FnTy.getReturnTypes().empty()) {
      return trap(ErrCode::Value::ComponentThreadStartInvalid);
    }
    auto *FnMut = const_cast<Runtime::Instance::FunctionInstance *>(Fn);
    // The body captures the executor, not this host function; its context is
    // the current one of the spawned thread.
    auto *Thread = TaskMgr.newSpawnThread(
        CurTask,
        [Ex = Exec, FnMut, C, Inst](Runtime::Component::ResumeReason Reason) {
          if (Reason == Runtime::Component::ResumeReason::Abort) {
            return;
          }
          auto *Ctx = Ex->getTaskManager().getCurrentContext();
          std::array<ValVariant, 1> A{ValVariant(C)};
          std::array<ValType, 1> Ty{ValType(TypeCode::I32)};
          auto Res = Ex->getCoreExecutor().invoke(FnMut, A, Ty);
          if (!Res &&
              Res.error().getEnum() != ErrCode::Value::ComponentAsyncAborted) {
            Ex->getTaskManager().noteTrap(Res.error(), Inst);
          }
          // Only a live instance is unregistered: a thread
          // outlives its call.
          if (Ctx->Index != 0 && !Ex->getTaskManager().isAborting()) {
            Inst->removeThread(Ctx->Index);
          }
          Ctx->Index = 0;
        });
    auto *Ctx = &Thread->SpawnContext;
    Ctx->Index = Inst->addThread(Ctx);
    Rets[0] = Ctx->Index;
    return {};
  }
  case ComponentCanonOpCode::Thread__resume_later: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->getThread(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Thread;
    if (V == nullptr || V->ReadyFn || !TaskMgr.isParked(V)) {
      return trap(ErrCode::Value::ComponentThreadNotSuspended);
    }
    V->ReadyFn = []() { return true; };
    return {};
  }
  case ComponentCanonOpCode::Thread__suspend: {
    if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
      return trapCannotBlock();
    }
    EXPECTED_TRY(auto Reason, Exec->getTaskManager().taskWait(
                                  *CurTask, nullptr, Info.isCancellable()));
    if (Reason == Runtime::Component::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Reason == Runtime::Component::ResumeReason::Cancelled
                  ? UINT32_C(1)
                  : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_resume: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->getThread(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Thread;
    if (V == nullptr || V->ReadyFn || !TaskMgr.isParked(V)) {
      return trap(ErrCode::Value::ComponentThreadNotSuspended);
    }
    // The resumed thread becomes ready first, so a sync task may hand over.
    V->ReadyFn = []() { return true; };
    if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
      return trapCannotBlock();
    }
    const bool SuspendSelf =
        Info.Code == ComponentCanonOpCode::Thread__suspend_then_resume;
    EXPECTED_TRY(auto Reason,
                 Exec->getTaskManager().taskWait(
                     *CurTask,
                     SuspendSelf ? std::function<bool()>()
                                 : std::function<bool()>([]() { return true; }),
                     Info.isCancellable()));
    if (Reason == Runtime::Component::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Reason == Runtime::Component::ResumeReason::Cancelled
                  ? UINT32_C(1)
                  : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Thread__yield_then_promote:
  case ComponentCanonOpCode::Thread__suspend_then_promote: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->getThread(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    // Hand over only to a ready waiter, else fall back to suspend / yield.
    auto *V = Ctx->Thread;
    const bool Waiting = V != nullptr && TaskMgr.isParked(V);
    if (Waiting && V->ReadyFn && V->ReadyFn()) {
      V->ReadyFn = []() { return true; };
    }
    if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
      return trapCannotBlock();
    }
    const bool SuspendSelf =
        Info.Code == ComponentCanonOpCode::Thread__suspend_then_promote;
    EXPECTED_TRY(auto Reason,
                 Exec->getTaskManager().taskWait(
                     *CurTask,
                     SuspendSelf ? std::function<bool()>()
                                 : std::function<bool()>([]() { return true; }),
                     Info.isCancellable()));
    if (Reason == Runtime::Component::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Reason == Runtime::Component::ResumeReason::Cancelled
                  ? UINT32_C(1)
                  : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Stream__new:
  case ComponentCanonOpCode::Future__new: {
    const bool IsStream = Info.Code == ComponentCanonOpCode::Stream__new;
    auto Shared =
        std::make_shared<Runtime::Instance::Component::TransmitState>();
    Shared->IsStream = IsStream;
    Shared->ElemType = Info.Elem;
    Shared->ElemTypeInst = Info.getElemInstance();
    auto ReadEnd = std::make_shared<Runtime::Instance::Component::TransmitEnd>(
        IsStream ? Runtime::Instance::Component::WaitableBase::Kind::StreamRead
                 : Runtime::Instance::Component::WaitableBase::Kind::FutureRead,
        Shared);
    auto WriteEnd = std::make_shared<Runtime::Instance::Component::TransmitEnd>(
        IsStream
            ? Runtime::Instance::Component::WaitableBase::Kind::StreamWrite
            : Runtime::Instance::Component::WaitableBase::Kind::FutureWrite,
        Shared);
    auto *ReadP = ReadEnd.get();
    auto *WriteP = WriteEnd.get();
    const uint32_t RIdx = Inst->addWaitable(std::move(ReadEnd));
    const uint32_t WIdx = Inst->addWaitable(std::move(WriteEnd));
    ReadP->TableIdx = RIdx;
    WriteP->TableIdx = WIdx;
    Rets[0] = (static_cast<uint64_t>(WIdx) << 32) | RIdx;
    return {};
  }

  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write:
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write:
    return runCopy(Args, Rets);

  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write:
    return runCancelCopy(Args, Rets);

  case ComponentCanonOpCode::Stream__drop_readable:
  case ComponentCanonOpCode::Stream__drop_writable:
  case ComponentCanonOpCode::Future__drop_readable:
  case ComponentCanonOpCode::Future__drop_writable:
    return runDropEnd(Args);

  case ComponentCanonOpCode::Error_context__new: {
    const LiftLowerContext Ctx{Info.Opts, Exec};
    const uint64_t Ptr = Ctx.liftPtr(Args[0]);
    const uint64_t Len = Ctx.liftPtr(Args[1]);
    std::string Msg;
    if (Info.Opts.Mem != nullptr && Len > 0) {
      if (Len > LiftLowerContext::MaxCanonByteLength ||
          !Info.Opts.Mem->checkAccessBound(Ptr, Len)) {
        return trapMsg(ErrCode::Value::ComponentTrap,
                       "error-context message out of bounds"sv);
      }
      const auto SpanBytes = Info.Opts.Mem->getSpan<const char>(Ptr, Len);
      Msg.assign(SpanBytes.begin(), SpanBytes.end());
    }
    Rets[0] = Inst->addErrorContext(std::move(Msg));
    return {};
  }
  case ComponentCanonOpCode::Error_context__debug_message: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    LiftLowerContext Ctx{Info.Opts, Exec};
    const uint64_t Ptr = Ctx.liftPtr(Args[1]);
    auto *Message = Inst->getErrorContext(Idx);
    if (Message == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    return Ctx.store(ComponentValVariant(*Message),
                     ComponentValType(ComponentTypeCode::String), Ptr);
  }
  case ComponentCanonOpCode::Error_context__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    if (!Inst->removeErrorContext(Idx)) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    return {};
  }

  default:
    return trapMsg(ErrCode::Value::ComponentTrap,
                   "canonical built-in is not implemented"sv);
  }
}

Expect<void> CanonAsyncBuiltinHostFunc::runCopy(Span<const ValVariant> Args,
                                                Span<ValVariant> Rets) {
  const auto *Inst = Info.Opts.Inst;
  Runtime::Component::Task *CurTask = Exec->getTaskManager().getCurrentTask();
  const bool IsWrite = Info.Code == ComponentCanonOpCode::Stream__write ||
                       Info.Code == ComponentCanonOpCode::Future__write;
  const bool IsFuture = Info.Code == ComponentCanonOpCode::Future__read ||
                        Info.Code == ComponentCanonOpCode::Future__write;
  const auto WantKind =
      IsFuture
          ? (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::FutureWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::FutureRead)
          : (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::StreamWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::
                       StreamRead);

  // A sync operation in a task that cannot block traps before the handle.
  if (!Info.isAsync() && (CurTask == nullptr || !Exec->mayBlock(*CurTask))) {
    return trapCannotBlock();
  }

  const LiftLowerContext Ctx{Info.Opts, Exec};
  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint64_t Ptr = Ctx.liftPtr(Args[1]);
  const uint64_t Count = IsFuture ? 1 : Ctx.liftPtr(Args[2]);
  if (Count > UINT32_MAX) {
    return trapMsg(ErrCode::Value::ComponentTrap,
                   "stream element count out of range"sv);
  }
  const uint32_t Len = static_cast<uint32_t>(Count);
  // A stream copy reports its result in the address type; a future in i32.
  auto Result = [&Ctx, IsFuture](uint32_t V) {
    return IsFuture ? ValVariant(V) : Ctx.lowerPtr(V);
  };

  auto *W = Inst->getWaitable(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<Runtime::Instance::Component::TransmitEnd *>(W);
  auto Shared = E->Shared;
  if (!LiftLowerContext::valTypeEq(Info.getElemInstance(), Info.Elem,
                                   Shared->ElemTypeInst, Shared->ElemType)) {
    return trapMsg(ErrCode::Value::ComponentHandleWrongType,
                   "stream or future element type mismatch"sv);
  }
  if (E->Status == Runtime::Instance::Component::TransmitEnd::State::Done) {
    if (IsFuture) {
      if (IsWrite) {
        return E->DoneByDrop
                   ? trapMsg(
                         ErrCode::Value::ComponentFutureWriteAfterSuccessOrDrop,
                         "cannot write to future after previous write "
                         "succeeded or readable end dropped"sv)
                   : trap(ErrCode::Value::ComponentFutureWriteAfterSuccess);
      }
      return trap(ErrCode::Value::ComponentFutureReadAfterSuccess);
    }
    if (IsWrite) {
      return trapMsg(ErrCode::Value::ComponentStreamWriteAfterDrop,
                     "cannot write to stream after being notified that the "
                     "readable end dropped"sv);
    }
    return trapMsg(ErrCode::Value::ComponentStreamReadAfterDrop,
                   "cannot read from stream after being notified that the "
                   "writable end dropped"sv);
  }
  if (E->isCopying()) {
    return trapMsg(
        ErrCode::Value::ComponentCopyBusy,
        "cannot have concurrent operations active on a future/stream"sv);
  }
  if (E->isInWaitableSet() && !Info.isAsync()) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }

  // Build and validate this side's buffer.
  E->Buffer = Runtime::Instance::Component::TransmitBuffer{
      {Inst, Info.Opts.Mem, Info.Opts.Realloc, nullptr, nullptr, Info.Opts.Enc},
      Info.Elem,
      Info.getElemInstance(),
      Ptr,
      Len,
      0,
      nullptr};
  EXPECTED_TRY(checkBuffer(Exec, E->Buffer));
  EXPECTED_TRY(Exec->startCopy(*E));

  if (!E->hasPendingEvent()) {
    if (!Info.isAsync()) {
      if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
        return trapCannotBlock();
      }
      E->HasSyncWaiter = true;
      auto ReasonOrErr = Exec->getTaskManager().taskWait(
          *CurTask, [E]() { return E->hasPendingEvent(); },
          /*Cancellable=*/false,
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      E->HasSyncWaiter = false;
    } else {
      Rets[0] = Result(Runtime::Instance::Component::TransmitBlocked);
      return {};
    }
  }
  const auto Ev = E->takePendingEvent();
  Rets[0] = Result(Ev.P2);
  return {};
}

Expect<void>
CanonAsyncBuiltinHostFunc::runCancelCopy(Span<const ValVariant> Args,
                                         Span<ValVariant> Rets) {
  const auto *Inst = Info.Opts.Inst;
  Runtime::Component::Task *CurTask = Exec->getTaskManager().getCurrentTask();
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__cancel_write ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__cancel_read ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const auto WantKind =
      IsFuture
          ? (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::FutureWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::FutureRead)
          : (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::StreamWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::
                       StreamRead);
  // A sync cancel in a task that cannot block traps before the handle.
  if (!Info.isAsync() && (CurTask == nullptr || !Exec->mayBlock(*CurTask))) {
    return trapCannotBlock();
  }
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->getWaitable(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<Runtime::Instance::Component::TransmitEnd *>(W);
  if (E->Status != Runtime::Instance::Component::TransmitEnd::State::Copying ||
      E->HasSyncWaiter) {
    return IsWrite ? trap(ErrCode::Value::ComponentCancelWriteNotPending)
                   : trap(ErrCode::Value::ComponentCancelReadNotPending);
  }
  if (E->isInWaitableSet() && !Info.isAsync()) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }
  EXPECTED_TRY(Exec->drainPostedTransmits());
  E->Status = Runtime::Instance::Component::TransmitEnd::State::CancellingCopy;
  auto Shared = E->Shared;
  if (!E->hasPendingEvent()) {
    // Cancel this side's parked rendezvous.
    if (Shared->PendingEnd == E) {
      Shared->PendingEnd = nullptr;
      E->queueCopyEvent(Runtime::Instance::Component::TransmitResult::Cancelled,
                        false);
    }
    if (!E->hasPendingEvent()) {
      if (!Info.isAsync()) {
        if (CurTask == nullptr || !Exec->mayBlock(*CurTask)) {
          return trapCannotBlock();
        }
        E->HasSyncWaiter = true;
        auto ReasonOrErr = Exec->getTaskManager().taskWait(
            *CurTask, [E]() { return E->hasPendingEvent(); },
            /*Cancellable=*/false);
        EXPECTED_TRY(auto Reason, ReasonOrErr);
        if (Reason == Runtime::Component::ResumeReason::Abort) {
          return Unexpect(ErrCode::Value::ComponentAsyncAborted);
        }
        E->HasSyncWaiter = false;
      } else {
        Rets[0] = Runtime::Instance::Component::TransmitBlocked;
        return {};
      }
    }
  }
  const auto Ev = E->takePendingEvent();
  uint32_t Payload = Ev.P2;
  // A cancelled stream copy reports its progress; a future keeps completion.
  if (!IsFuture &&
      (Payload & 0xFU) ==
          static_cast<uint32_t>(
              Runtime::Instance::Component::TransmitResult::Completed)) {
    Payload = (Payload & ~0xFU) |
              static_cast<uint32_t>(
                  Runtime::Instance::Component::TransmitResult::Cancelled);
  }
  Rets[0] = Payload;
  return {};
}

Expect<void>
CanonAsyncBuiltinHostFunc::runDropEnd(Span<const ValVariant> Args) {
  const auto *Inst = Info.Opts.Inst;
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__drop_writable ||
      Info.Code == ComponentCanonOpCode::Future__drop_writable;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__drop_readable ||
      Info.Code == ComponentCanonOpCode::Future__drop_writable;
  const auto WantKind =
      IsFuture
          ? (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::FutureWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::FutureRead)
          : (IsWrite
                 ? Runtime::Instance::Component::WaitableBase::Kind::StreamWrite
                 : Runtime::Instance::Component::WaitableBase::Kind::
                       StreamRead);
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->getWaitable(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<Runtime::Instance::Component::TransmitEnd *>(W);
  auto Shared = E->Shared;
  if (!LiftLowerContext::valTypeEq(Info.getElemInstance(), Info.Elem,
                                   Shared->ElemTypeInst, Shared->ElemType)) {
    return trapMsg(ErrCode::Value::ComponentHandleWrongType,
                   "stream or future element type mismatch"sv);
  }
  if (E->isCopying()) {
    // The readable end reports "remove", the writable end "drop".
    return IsWrite ? trap(ErrCode::Value::ComponentStreamDropBusy)
                   : trap(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  if (IsFuture && IsWrite &&
      E->Status != Runtime::Instance::Component::TransmitEnd::State::Done) {
    return trapMsg(
        ErrCode::Value::ComponentFutureWriteEndNoValue,
        "cannot drop future write end without first writing a value"sv);
  }
  E->drop();
  Inst->removeWaitable(Idx);
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
