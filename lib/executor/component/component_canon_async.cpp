// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
uint64_t getPtrArg(const Component::CanonFunction &Canon,
                   const ValVariant &Val) {
  return Canon.isMemory64() ? Val.get<uint64_t>()
                            : static_cast<uint64_t>(Val.get<uint32_t>());
}
} // namespace

// Write an event to memory. See "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::storeEvent(const Component::CanonFunction &Canon,
                              const Event &Pending, uint64_t Ptr) {
  auto *Mem = Canon.getOptions().Mem;
  if (Mem == nullptr) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    the canonical option `memory` is required"sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (Ptr % 4 != 0) {
    spdlog::error(ErrCode::Value::ComponentPtrUnaligned);
    return Unexpect(ErrCode::Value::ComponentPtrUnaligned);
  }
  EXPECTED_TRY(Mem->storeValue(Pending.Idx, Ptr));
  EXPECTED_TRY(Mem->storeValue(Pending.Payload, Ptr + 4));
  return static_cast<uint32_t>(Pending.Code);
}

// backpressure.inc / backpressure.dec. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonBackpressure(const Component::CanonFunction &Canon,
                                        bool Inc) {
  const auto *Inst = Canon.getInstance();
  const bool Ok = Inc ? Inst->incBackpressure() : Inst->decBackpressure();
  if (!Ok) {
    spdlog::error(ErrCode::Value::ComponentBackpressureOverflow);
    return Unexpect(ErrCode::Value::ComponentBackpressureOverflow);
  }
  return {};
}

// task.return. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonTaskReturn(const Component::CanonFunction &Canon,
                                      Span<const ValVariant> Args) {
  Runtime::Component::Task *T = getCurrentTask();
  if (T == nullptr || T->getFuncType() == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const auto &Opts = Canon.getOptions();
  const auto &TaskOpts = T->getCanonOptions();
  // Only an async-lifted task returns this way, with the result type and
  // the options it was lifted with.
  bool Valid = TaskOpts.Async && Opts.Encoding == TaskOpts.Encoding &&
               (Opts.Mem == nullptr || Opts.Mem == TaskOpts.Mem);
  const auto Results = T->getFuncType()->getResultValTypes();
  const auto Declared = Canon.getResultTypes();
  Valid = Valid && Results.size() == Declared.size();
  for (size_t I = 0; Valid && I < Results.size(); ++I) {
    EXPECTED_TRY(const bool Same,
                 matchValType(*Canon.getTypeInstance(), Declared[I],
                              *T->getTypeInstance(), Results[I]));
    Valid = Same;
  }
  if (!Valid) {
    spdlog::error(ErrCode::Value::ComponentTaskReturnInvalid);
    return Unexpect(ErrCode::Value::ComponentTaskReturnInvalid);
  }
  EXPECTED_TRY(auto Vals, liftValues(Opts, *Canon.getTypeInstance(), Declared,
                                     Args, MaxFlatParams));
  return T->onReturn(std::move(Vals));
}

// task.cancel. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonTaskCancel(const Component::CanonFunction &) {
  Runtime::Component::Task *T = getCurrentTask();
  if (T == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  // Only an async-lifted task cancels itself this way.
  if (!T->getCanonOptions().Async) {
    spdlog::error(ErrCode::Value::ComponentTaskNotCancelled);
    return Unexpect(ErrCode::Value::ComponentTaskNotCancelled);
  }
  return T->onCancel();
}

// context.get. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonContextGet(const Component::CanonFunction &Canon,
                                      Span<ValVariant> Rets) {
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const uint64_t V = Cur->getStorage(Canon.getConstVal());
  if (Canon.getFuncType().getReturnTypes()[0].getCode() == TypeCode::I64) {
    Rets[0] = ValVariant(V);
  } else {
    Rets[0] = ValVariant(static_cast<uint32_t>(V));
  }
  return {};
}

// context.set. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonContextSet(const Component::CanonFunction &Canon,
                                      Span<const ValVariant> Args) {
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  if (Canon.getFuncType().getParamTypes()[0].getCode() == TypeCode::I64) {
    Cur->setStorage(Canon.getConstVal(), Args[0].get<uint64_t>());
  } else {
    Cur->setStorage(Canon.getConstVal(), Args[0].get<uint32_t>());
  }
  return {};
}

// yield. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonYield(const Component::CanonFunction &Canon,
                                 Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  // A yield reports whether the task was cancelled. One that may not block
  // completes in place, still delivering a queued cancellation.
  const bool Cancellable = Canon.getFlag();
  if (!T->mayBlock(*Cur, Waiting)) {
    Rets[0] = ValVariant(T->onCancelDelivery(Cancellable) ? UINT32_C(1)
                                                          : UINT32_C(0));
    return {};
  }
  const Runtime::Component::Thread::Reason R = T->yield(*Cur, Cancellable);
  if (R == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(R == Runtime::Component::Thread::Reason::Cancelled
                           ? UINT32_C(1)
                           : UINT32_C(0));
  return {};
}

// subtask.cancel. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonSubtaskCancel(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args,
                                         Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const bool Async = Canon.getFlag();
  // A synchronous cancel may have to block, which is decided first.
  if (!Async) {
    EXPECTED_TRY(checkMayBlock(*T, *Cur));
  }
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  EXPECTED_TRY(Runtime::Component::Task * Callee, getSubtask(*Inst, Idx));
  if (Callee->isDelivered()) {
    spdlog::error(ErrCode::Value::ComponentSubtaskCancelTerminal);
    spdlog::error("    the subtask already delivered its terminal status"sv);
    return Unexpect(ErrCode::Value::ComponentSubtaskCancelTerminal);
  }
  // A subtask takes one cancellation request only.
  if (Callee->isCancelRequested()) {
    spdlog::error(ErrCode::Value::ComponentSubtaskCancelTerminal);
    spdlog::error("    the subtask was already asked to cancel"sv);
    return Unexpect(ErrCode::Value::ComponentSubtaskCancelTerminal);
  }
  if (!Async && Callee->getSetIdx() != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (!Callee->isResolved()) {
    // Ask the callee to stop; a thread parked cancellably wakes now.
    if (Runtime::Component::Thread *Wake = Callee->onCancelRequest();
        Wake != nullptr) {
      resume(*Wake, Runtime::Component::Thread::Reason::Cancelled);
    }
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
    if (!Callee->isResolved()) {
      if (Async) {
        Rets[0] = ValVariant(TransmitBlocked);
        return {};
      }
      const Runtime::Component::Thread::Reason R = T->wait(
          *Cur, [Callee]() { return Callee->isResolved(); },
          /*Cancellable=*/false);
      if (R == Runtime::Component::Thread::Reason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
  }
  // The resolution is delivered here.
  if (Inst->findSubtask(Idx) != Callee) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  const uint32_t Code = Callee->getSubtaskCode();
  Callee->setDeliveredCode(Code);
  Callee->setDelivered();
  Callee->releaseLenders();
  Rets[0] = ValVariant(Code);
  return {};
}

// subtask.drop. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonSubtaskDrop(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args) {
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  EXPECTED_TRY(Runtime::Component::Task * Callee, getSubtask(*Inst, Idx));
  if (!Callee->isDelivered()) {
    spdlog::error(ErrCode::Value::ComponentSubtaskNotResolved);
    return Unexpect(ErrCode::Value::ComponentSubtaskNotResolved);
  }
  if (Callee->getSetIdx() != 0) {
    EXPECTED_TRY(joinWaitableSet(*Inst, Idx, 0));
  }
  Inst->removeSubtask(Idx);
  return {};
}

// stream.new / future.new. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonStreamNew(const Component::CanonFunction &Canon,
                                     Span<ValVariant> Rets, bool IsStream) {
  const auto *Inst = Canon.getInstance();
  auto S = std::make_shared<Runtime::Instance::Component::Stream>(
      !IsStream, Canon.getValType(), Canon.getTypeInstance());
  const uint64_t ReadIdx = Inst->addStreamEnd(
      S, Runtime::Instance::Component::Stream::End::Readable);
  const uint64_t WriteIdx = Inst->addStreamEnd(
      S, Runtime::Instance::Component::Stream::End::Writable);
  Rets[0] = ValVariant((WriteIdx << 32) | ReadIdx);
  return {};
}

// The copy of one end. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::copy(Runtime::Instance::Component::Stream &S,
                        Runtime::Instance::Component::Stream::End E) {
  const uint32_t N = S.onCopy(E);
  if (N > 0) {
    EXPECTED_TRY(copyElements(S, N));
    S.onMove(N);
  }
  return {};
}

// Move elements between the two buffers. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::copyElements(Runtime::Instance::Component::Stream &S,
                                uint32_t Count) {
  auto &SrcBuffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Writable);
  auto &DstBuffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Readable);
  const auto &Elem = S.getElemType();
  if (Count == 0 || !Elem.has_value()) {
    // No payload: only the count moves.
    return {};
  }
  const bool Bytes =
      Elem->isPrimValType() && Elem->getPrimValType() == PrimValType::U8;
  // Every element is read before any is written, so overlapping guest
  // buffers move as a whole.
  std::vector<ComponentValVariant> Vals;
  std::vector<uint8_t> Raw;
  if (SrcBuffer.Host) {
    const size_t Start = SrcBuffer.Base + SrcBuffer.Progress;
    if (Bytes && !SrcBuffer.Bytes.empty()) {
      Raw.assign(SrcBuffer.Bytes.begin() + Start,
                 SrcBuffer.Bytes.begin() + Start + Count);
    } else {
      Vals.assign(SrcBuffer.Vals.begin() + Start,
                  SrcBuffer.Vals.begin() + Start + Count);
    }
  } else {
    const bool IsMem64 = SrcBuffer.Opts.Mem != nullptr &&
                         SrcBuffer.Opts.Mem->getMemoryType().getLimit().is64();
    EXPECTED_TRY(const uint64_t Size,
                 getElemSize(*SrcBuffer.ElemInst, *SrcBuffer.Elem, IsMem64));
    if (Bytes) {
      EXPECTED_TRY(auto Bytes2,
                   SrcBuffer.Opts.Mem->getBytes(
                       SrcBuffer.Ptr + SrcBuffer.Progress * Size, Count));
      Raw.assign(Bytes2.begin(), Bytes2.end());
    } else {
      for (uint32_t I = 0; I < Count; ++I) {
        EXPECTED_TRY(auto Val,
                     load(SrcBuffer.Opts, *SrcBuffer.ElemInst, *SrcBuffer.Elem,
                          SrcBuffer.Ptr + (SrcBuffer.Progress + I) * Size));
        Vals.push_back(std::move(Val));
      }
    }
  }
  if (Bytes && Raw.empty()) {
    for (const auto &V : Vals) {
      Raw.push_back(std::get<uint8_t>(V));
    }
  }
  if (DstBuffer.Host) {
    if (Bytes) {
      DstBuffer.Bytes.resize(std::max<size_t>(
          DstBuffer.Bytes.size(), DstBuffer.Base + DstBuffer.Progress + Count));
      std::copy(Raw.begin(), Raw.end(),
                DstBuffer.Bytes.begin() + DstBuffer.Base + DstBuffer.Progress);
    } else {
      DstBuffer.Vals.resize(std::max<size_t>(
          DstBuffer.Vals.size(), DstBuffer.Base + DstBuffer.Progress + Count));
      std::move(Vals.begin(), Vals.end(),
                DstBuffer.Vals.begin() + DstBuffer.Base + DstBuffer.Progress);
    }
    return {};
  }
  const bool IsMem64 = DstBuffer.Opts.Mem != nullptr &&
                       DstBuffer.Opts.Mem->getMemoryType().getLimit().is64();
  EXPECTED_TRY(const uint64_t Size,
               getElemSize(*DstBuffer.ElemInst, *DstBuffer.Elem, IsMem64));
  if (Bytes) {
    EXPECTED_TRY(DstBuffer.Opts.Mem->setBytes(
        Raw, DstBuffer.Ptr + DstBuffer.Progress * Size, 0, Raw.size()));
    return {};
  }
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(checkValue(*DstBuffer.ElemInst, *DstBuffer.Elem, Vals[I]));
    EXPECTED_TRY(store(DstBuffer.Opts, *DstBuffer.ElemInst, *DstBuffer.Elem,
                       Vals[I],
                       DstBuffer.Ptr + (DstBuffer.Progress + I) * Size));
  }
  return {};
}

// The outcome of a copy built-in. See
// "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::takeCopyOutcome(Runtime::Instance::Component::Stream &S,
                                   Runtime::Instance::Component::Stream::End E,
                                   bool IsAsync) {
  if (S.hasPending(E)) {
    return takeOutcome(S, E);
  }
  if (IsAsync) {
    return TransmitBlocked;
  }
  // A synchronous copy waits for its outcome on the current thread.
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  S.setSyncWaiter(E, true);
  const Runtime::Component::Thread::Reason R = T->collect(
      *Cur, [&S, E]() { return S.hasPending(E); }, /*Cancellable=*/false);
  if (R == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  S.setSyncWaiter(E, false);
  return takeOutcome(S, E);
}

// stream.read / stream.write / future.read / future.write. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonStreamCopy(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsStream, bool IsRead) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  // The `async` canonical option selects the non-blocking copy.
  const auto &Opts = Canon.getOptions();
  const bool Async = Opts.Async;
  // A synchronous copy may have to block, which is decided first.
  if (!Async) {
    EXPECTED_TRY(checkMayBlock(*T, *Cur));
  }
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint64_t Ptr = getPtrArg(Canon, Args[1]);
  const uint32_t Count =
      IsStream ? static_cast<uint32_t>(getPtrArg(Canon, Args[2])) : 1;
  const Runtime::Instance::Component::Stream::End E =
      IsRead ? Runtime::Instance::Component::Stream::End::Readable
             : Runtime::Instance::Component::Stream::End::Writable;
  EXPECTED_TRY(Runtime::Instance::Component::Stream * SP,
               getStreamEnd(*Inst, Idx, !IsStream, E));
  Runtime::Instance::Component::Stream &S = *SP;
  if (S.isBusy(E)) {
    spdlog::error(ErrCode::Value::ComponentCopyBusy);
    return Unexpect(ErrCode::Value::ComponentCopyBusy);
  }
  if (S.isDone(E)) {
    ErrCode::Value Code = ErrCode::Value::ComponentFutureWriteAfterSuccess;
    if (IsStream) {
      Code = IsRead ? ErrCode::Value::ComponentStreamReadAfterDrop
                    : ErrCode::Value::ComponentStreamWriteAfterDrop;
    } else if (IsRead) {
      Code = ErrCode::Value::ComponentFutureReadAfterSuccess;
    } else if (S.isDoneByDrop(E)) {
      Code = ErrCode::Value::ComponentFutureWriteAfterSuccessOrDrop;
    }
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (!Async && S.getSetIdx(E) != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (Count > MaxCopyLength) {
    spdlog::error(ErrCode::Value::ComponentStreamOpTooBig);
    return Unexpect(ErrCode::Value::ComponentStreamOpTooBig);
  }
  const auto &Elem = Canon.getValType();
  // Both ends within one instance cannot meet over elements that need
  // lifting on the way.
  const Runtime::Instance::Component::Stream::End P =
      Runtime::Instance::Component::Stream::getPeer(E);
  if (S.isParked(P) && !S.getBuffer(P).Host &&
      S.getBuffer(P).Opts.Inst == Inst && Elem.has_value()) {
    EXPECTED_TRY(const bool Raw,
                 isRawCopyable(*Canon.getTypeInstance(), *Elem));
    if (!Raw) {
      spdlog::error(ErrCode::Value::ComponentIntraCopy);
      return Unexpect(ErrCode::Value::ComponentIntraCopy);
    }
  }
  // The buffer: the elements of the type immediate in the memory of the
  // options.
  if (Count > 0 && Elem.has_value()) {
    EXPECTED_TRY(const uint32_t Align, getAlignment(*Canon.getTypeInstance(),
                                                    *Elem, Canon.isMemory64()));
    EXPECTED_TRY(const uint64_t Size, getElemSize(*Canon.getTypeInstance(),
                                                  *Elem, Canon.isMemory64()));
    EXPECTED_TRY(checkAligned(Ptr, Align));
    EXPECTED_TRY(checkInBounds(Opts, Ptr, Count * Size));
  }
  Runtime::Instance::Component::Stream::Buffer B;
  B.Opts = Opts;
  B.Elem = Elem;
  B.ElemInst = Canon.getTypeInstance();
  B.Ptr = Ptr;
  B.Length = Count;
  S.getBuffer(E) = std::move(B);
  EXPECTED_TRY(copy(S, E));
  EXPECTED_TRY(const uint32_t Result, takeCopyOutcome(S, E, Async));
  Rets[0] = Canon.isMemory64() && IsStream
                ? ValVariant(static_cast<uint64_t>(Result))
                : ValVariant(Result);
  return {};
}

// stream.cancel-read / stream.cancel-write / future.cancel-read /
// future.cancel-write. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonStreamCancel(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsStream, bool IsRead) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const bool Async = Canon.getFlag();
  if (!Async) {
    EXPECTED_TRY(checkMayBlock(*T, *Cur));
  }
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const Runtime::Instance::Component::Stream::End E =
      IsRead ? Runtime::Instance::Component::Stream::End::Readable
             : Runtime::Instance::Component::Stream::End::Writable;
  EXPECTED_TRY(Runtime::Instance::Component::Stream * SP,
               getStreamEnd(*Inst, Idx, !IsStream, E));
  Runtime::Instance::Component::Stream &S = *SP;
  if (!Async && S.getSetIdx(E) != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (!S.isBusy(E)) {
    const ErrCode::Value Code =
        IsRead ? ErrCode::Value::ComponentCancelReadNotPending
               : ErrCode::Value::ComponentCancelWriteNotPending;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  S.onCancel(E);
  Rets[0] = ValVariant(takeOutcome(S, E));
  return {};
}

// stream.drop-readable / stream.drop-writable / future.drop-readable /
// future.drop-writable. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonStreamDrop(const Component::CanonFunction &Canon,
                                      Span<const ValVariant> Args,
                                      bool IsStream, bool IsRead) {
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const Runtime::Instance::Component::Stream::End E =
      IsRead ? Runtime::Instance::Component::Stream::End::Readable
             : Runtime::Instance::Component::Stream::End::Writable;
  EXPECTED_TRY(Runtime::Instance::Component::Stream * SP,
               getStreamEnd(*Inst, Idx, !IsStream, E));
  Runtime::Instance::Component::Stream &S = *SP;
  if (S.isBusy(E)) {
    const ErrCode::Value Code = IsRead
                                    ? ErrCode::Value::ComponentStreamRemoveBusy
                                    : ErrCode::Value::ComponentStreamDropBusy;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (!IsStream && !IsRead && !S.isDone(E) && !S.isPeerDropped(E)) {
    spdlog::error(ErrCode::Value::ComponentFutureWriteEndNoValue);
    return Unexpect(ErrCode::Value::ComponentFutureWriteEndNoValue);
  }
  if (S.getSetIdx(E) != 0) {
    EXPECTED_TRY(joinWaitableSet(*Inst, Idx, 0));
  }
  S.onDrop(E);
  Inst->removeStreamEnd(Idx);
  return {};
}

// error-context.new. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonErrorContextNew(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets) {
  const uint64_t Ptr = getPtrArg(Canon, Args[0]);
  const uint64_t Len = getPtrArg(Canon, Args[1]);
  EXPECTED_TRY(auto Msg, loadString(Canon.getOptions(), Ptr, Len));
  Rets[0] = ValVariant(Canon.getInstance()->addErrorContext(std::move(Msg)));
  return {};
}

// error-context.debug-message. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonErrorContextDebugMessage(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  const auto &Opts = Canon.getOptions();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint64_t Ptr = getPtrArg(Canon, Args[1]);
  const auto *Msg = Canon.getInstance()->findErrorContext(Idx);
  if (Msg == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  EXPECTED_TRY(auto Range, storeString(Opts, *Msg));
  EXPECTED_TRY(storePtr(Opts, Ptr, Range.first));
  return storePtr(Opts, Ptr + (Canon.isMemory64() ? 8 : 4), Range.second);
}

// error-context.drop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonErrorContextDrop(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  if (!Canon.getInstance()->removeErrorContext(Args[0].get<uint32_t>())) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  return {};
}

// waitable-set.new. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonWaitableSetNew(const Component::CanonFunction &Canon,
                                          Span<ValVariant> Rets) {
  Rets[0] = ValVariant(Canon.getInstance()->addWaitableSet());
  return {};
}

// waitable-set.wait / waitable-set.poll. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonWaitableSetWait(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsPoll) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const bool Cancellable = Canon.getFlag();
  const auto *Inst = Canon.getInstance();
  const uint32_t SetIdx = Args[0].get<uint32_t>();
  const uint64_t Ptr = getPtrArg(Canon, Args[1]);
  auto *Set = Inst->findWaitableSet(SetIdx);
  if (Set == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto FirstPending = [this, Inst, SetIdx]() -> std::optional<Event> {
    const auto *Members = Inst->findWaitableSet(SetIdx);
    if (Members == nullptr) {
      return std::nullopt;
    }
    for (const uint32_t Idx : *Members) {
      if (hasPendingEvent(*Inst, Idx)) {
        return takePendingEvent(*Inst, Idx);
      }
    }
    return std::nullopt;
  };
  if (!IsPoll) {
    EXPECTED_TRY(const Event Ev,
                 waitOnSet(*T, *Cur, *Inst, SetIdx, Cancellable));
    EXPECTED_TRY(const uint32_t Code, storeEvent(Canon, Ev, Ptr));
    Rets[0] = ValVariant(Code);
    return {};
  }
  // A poll yields once when nothing is pending and it may block, then reports
  // what there is; the event memory is written even when there is no event.
  std::optional<Event> Ev = FirstPending();
  if (!Ev.has_value()) {
    if (T->mayBlock(*Cur, Waiting)) {
      const Runtime::Component::Thread::Reason R = T->yield(*Cur, Cancellable);
      if (R == Runtime::Component::Thread::Reason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      if (R == Runtime::Component::Thread::Reason::Cancelled) {
        Ev = Event{EventCode::TaskCancelled, 0, 0};
      } else {
        Ev = FirstPending();
      }
    } else if (T->onCancelDelivery(Cancellable)) {
      Ev = Event{EventCode::TaskCancelled, 0, 0};
    }
  }
  if (!Ev.has_value()) {
    Ev = Event{EventCode::None, 0, 0};
  }
  EXPECTED_TRY(const uint32_t Code, storeEvent(Canon, *Ev, Ptr));
  Rets[0] = ValVariant(Code);
  return {};
}

// waitable-set.drop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonWaitableSetDrop(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  const auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *Members = Inst->findWaitableSet(Idx);
  if (Members == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  // A set stays while a thread of the instance waits on it.
  bool Waited = false;
  for (const Runtime::Component::Thread *T : Inst->getThreads()) {
    Waited = Waited || (T != nullptr && T->getWaitingSet() == Idx);
  }
  if (Waited || !Members->empty()) {
    spdlog::error(ErrCode::Value::ComponentWaitableSetNotEmpty);
    return Unexpect(ErrCode::Value::ComponentWaitableSetNotEmpty);
  }
  Inst->removeWaitableSet(Idx);
  return {};
}

// waitable.join. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonWaitableJoin(const Component::CanonFunction &Canon,
                                        Span<const ValVariant> Args) {
  return joinWaitableSet(*Canon.getInstance(), Args[0].get<uint32_t>(),
                         Args[1].get<uint32_t>());
}

// thread.index. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonThreadIndex(const Component::CanonFunction &,
                                       Span<ValVariant> Rets) {
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  Rets[0] = ValVariant(Cur->getIndex());
  return {};
}

// thread.new-indirect. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonThreadNewIndirect(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask();
  if (T == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  auto *Tab = Canon.getTable();
  const uint64_t FuncIdx = Tab->getTableType().getLimit().is64()
                               ? Args[0].get<uint64_t>()
                               : static_cast<uint64_t>(Args[0].get<uint32_t>());
  const uint32_t Context = Args[1].get<uint32_t>();
  EXPECTED_TRY(auto Ref, Tab->getRefAddr(FuncIdx));
  const auto *Func = Ref.getPtr<Runtime::Instance::FunctionInstance>();
  const ValType I32V(TypeCode::I32);
  if (Func == nullptr ||
      Func->getFuncType().getParamTypes() != std::vector<ValType>{I32V} ||
      !Func->getFuncType().getReturnTypes().empty()) {
    spdlog::error(ErrCode::Value::ComponentThreadStartInvalid);
    return Unexpect(ErrCode::Value::ComponentThreadStartInvalid);
  }
  // A new thread of the task on a stack of its own, suspended until resumed.
  Runtime::Component::Thread *Cur = newThread(*T, [this, T, Func, Context]() {
    Runtime::Component::Thread *Cur = getCurrentThread();
    assuming(Cur != nullptr);
    auto Res = invokeCore(Func, std::vector<ValVariant>{ValVariant(Context)});
    if (Res) {
      auto Left = T->removeThread(*Cur);
      if (!Left) {
        setTrap(Left.error(), T->getInstance());
      }
      return;
    }
    if (Res.error() != ErrCode::Value::ComponentAsyncAborted) {
      setTrap(Res.error(), T->getInstance());
    }
  });
  T->addThread(*Cur);
  Rets[0] = ValVariant(Cur->getIndex());
  return {};
}

// thread.resume-later. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonThreadResumeLater(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  Runtime::Component::Thread *Other =
      Canon.getInstance()->findThread(Args[0].get<uint32_t>());
  if (Other == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (!Other->isSuspended()) {
    spdlog::error(ErrCode::Value::ComponentThreadNotSuspended);
    return Unexpect(ErrCode::Value::ComponentThreadNotSuspended);
  }
  Other->onReady();
  if (std::find(Waiting.begin(), Waiting.end(), Other) == Waiting.end()) {
    Waiting.push_back(Other);
  }
  return {};
}

// thread.suspend. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonThreadSuspend(const Component::CanonFunction &Canon,
                                         Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  EXPECTED_TRY(checkMayBlock(*T, *Cur));
  const Runtime::Component::Thread::Reason R =
      T->suspend(*Cur, Canon.getFlag());
  if (R == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(R == Runtime::Component::Thread::Reason::Cancelled
                           ? UINT32_C(1)
                           : UINT32_C(0));
  return {};
}

// The four thread.{suspend,yield}-then-{resume,promote} built-ins. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonThreadSwitch(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsYield, bool IsPromote) {
  Runtime::Component::Task *T = getCurrentTask();
  Runtime::Component::Thread *Cur = getCurrentThread();
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  Runtime::Component::Thread *Other =
      Canon.getInstance()->findThread(Args[0].get<uint32_t>());
  if (Other == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  const Runtime::Component::Thread::Next N =
      IsYield ? Runtime::Component::Thread::Next::Waiting
              : Runtime::Component::Thread::Next::Suspended;
  Runtime::Component::Thread::Reason R;
  if (IsPromote) {
    // The named thread takes the stack only if it is ready; else this one
    // parks, which it must be allowed to.
    if (!Other->isReady()) {
      EXPECTED_TRY(checkMayBlock(*T, *Cur));
    }
    R = T->promote(*Cur, *Other, N, Canon.getFlag());
  } else {
    // The named thread takes the stack next, so nothing is blocked.
    if (!Other->isSuspended()) {
      spdlog::error(ErrCode::Value::ComponentThreadNotSuspended);
      return Unexpect(ErrCode::Value::ComponentThreadNotSuspended);
    }
    R = T->switchTo(*Cur, *Other, N, Canon.getFlag());
  }
  if (R == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(R == Runtime::Component::Thread::Reason::Cancelled
                           ? UINT32_C(1)
                           : UINT32_C(0));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
