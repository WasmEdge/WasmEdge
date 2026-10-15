// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Write an event to memory. See "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::storeEvent(const Component::CanonFunction &Canon,
                              const Component::WaitableEvent &Pending,
                              uint64_t Ptr) {
  EXPECTED_TRY(auto *Mem, Canon.getOptions().getMemory());
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
  auto *Inst = Canon.getInstance();
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
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  if (T == nullptr || T->getFuncType() == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const auto &Opts = Canon.getOptions();
  const auto &TaskOpts = T->getCanonOptions();
  // An async-lifted task returns this way, with its result type and options.
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
  EXPECTED_TRY(auto Vals, liftFlatValues(Opts, *Canon.getTypeInstance(),
                                         Declared, Args, MaxFlatParams));
  return T->onReturn(std::move(Vals));
}

// task.cancel. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonTaskCancel(const Component::CanonFunction &Canon) {
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
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
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
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
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
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
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  if (T->yield(*Cur) == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(UINT32_C(0));
  return {};
}

// subtask.cancel. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonSubtaskCancel(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args,
                                         Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const bool Async = Canon.getFlag();
  auto *Inst = Canon.getInstance();
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
  // A subtask in a waitable set is never cancelled, async or not.
  if (Callee->getSetIdx() != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (!Callee->isResolved()) {
    // The subtask cannot join a waitable set while this cancel runs.
    Callee->setSyncWaiter(true);
    if (Runtime::Component::Thread *Wake = Callee->onCancelRequest();
        Wake != nullptr) {
      resumeThread(*Wake, Runtime::Component::Thread::WakeReason::Normal);
    }
    if (const auto Trap =
            getTrap(Canon.getInstance()->getRoot()->getStoreRoots());
        Trap.has_value()) {
      return Unexpect(*Trap);
    }
    if (!Async && !Callee->isResolved()) {
      const Runtime::Component::Thread::WakeReason R =
          T->wait(*Cur, [Callee]() {
            return Callee->isResolved() || Callee->isAborted();
          });
      if (R == Runtime::Component::Thread::WakeReason::Aborted) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
    Callee->setSyncWaiter(false);
    if (!Callee->isResolved()) {
      Rets[0] = ValVariant(CopyBlocked);
      return {};
    }
  }
  // The resolution is delivered here.
  if (Inst->findSubtask(Idx) != Callee) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  Rets[0] = ValVariant(Callee->onCollect());
  return {};
}

// subtask.drop. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonSubtaskDrop(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args) {
  auto *Inst = Canon.getInstance();
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
  Callee->releaseDependent();
  return {};
}

// stream.new / future.new. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonStreamNew(const Component::CanonFunction &Canon,
                                     Span<ValVariant> Rets, bool IsStream) {
  auto *Inst = Canon.getInstance();
  auto *S = Inst->getRoot()->newStream(!IsStream, Canon.getValType(),
                                       Canon.getTypeInstance());
  const uint64_t ReadIdx = Inst->addStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Reader);
  const uint64_t WriteIdx = Inst->addStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Writer);
  Rets[0] = ValVariant((WriteIdx << 32) | ReadIdx);
  return {};
}

// The copy of one end. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::startStreamCopy(
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role) {
  const auto [Peer, N] = S.onCopy(Role);
  if (Peer == nullptr) {
    return {};
  }
  const auto PeerRole =
      Runtime::Instance::Component::StreamInstance::getPeerRole(Role);
  auto &Mine = S.getBuffer(Role);
  auto &Theirs = Peer->getBuffer(PeerRole);
  // Two ends held by one instance meet only over raw-copyable elements.
  if (!Mine.IsHost && !Theirs.IsHost && S.getHolder(Role) != nullptr &&
      S.getHolder(Role) == Peer->getHolder(PeerRole) &&
      Mine.ElemType.has_value()) {
    EXPECTED_TRY(const bool Raw,
                 isRawCopyable(*Mine.ElemTypeInst, *Mine.ElemType));
    if (!Raw) {
      spdlog::error(ErrCode::Value::ComponentIntraCopy);
      return Unexpect(ErrCode::Value::ComponentIntraCopy);
    }
  }
  const bool IsWriter =
      Role == Runtime::Instance::Component::StreamInstance::Role::Writer;
  EXPECTED_TRY(
      moveElements(IsWriter ? Mine : Theirs, IsWriter ? Theirs : Mine, N));
  S.onMove(Role, N);
  return {};
}

// Move elements between the two buffers. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::moveElements(
    Runtime::Instance::Component::StreamBuffer &SrcBuffer,
    Runtime::Instance::Component::StreamBuffer &DstBuffer, uint32_t Count) {
  const auto &Elem = SrcBuffer.ElemType;
  if (Count == 0 || !Elem.has_value()) {
    // No payload: only the count moves.
    return {};
  }
  const bool Bytes =
      Elem->isPrimValType() && Elem->getPrimValType() == PrimValType::U8;
  // Read every element before writing any, so overlapping buffers work.
  std::vector<ComponentValVariant> Vals;
  std::vector<uint8_t> Raw;
  if (SrcBuffer.IsHost) {
    const auto Start =
        static_cast<std::ptrdiff_t>(SrcBuffer.HostBase + SrcBuffer.Progress);
    const auto End = Start + static_cast<std::ptrdiff_t>(Count);
    if (Bytes && !SrcBuffer.HostBytes.empty()) {
      Raw.assign(SrcBuffer.HostBytes.begin() + Start,
                 SrcBuffer.HostBytes.begin() + End);
    } else {
      Vals.assign(SrcBuffer.HostVals.begin() + Start,
                  SrcBuffer.HostVals.begin() + End);
    }
  } else {
    const bool IsMem64 = SrcBuffer.Opts.isMemory64();
    EXPECTED_TRY(
        const uint64_t Size,
        getElemSize(*SrcBuffer.ElemTypeInst, *SrcBuffer.ElemType, IsMem64));
    if (Bytes) {
      EXPECTED_TRY(auto Bytes2,
                   SrcBuffer.Opts.Mem->getBytes(
                       SrcBuffer.Ptr + SrcBuffer.Progress * Size, Count));
      Raw.assign(Bytes2.begin(), Bytes2.end());
    } else {
      for (uint32_t I = 0; I < Count; ++I) {
        EXPECTED_TRY(auto Val,
                     load(SrcBuffer.Opts, *SrcBuffer.ElemTypeInst,
                          *SrcBuffer.ElemType,
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
  if (DstBuffer.IsHost) {
    if (Bytes) {
      DstBuffer.HostBytes.resize(
          std::max<size_t>(DstBuffer.HostBytes.size(),
                           DstBuffer.HostBase + DstBuffer.Progress + Count));
      std::copy(Raw.begin(), Raw.end(),
                DstBuffer.HostBytes.begin() + DstBuffer.HostBase +
                    DstBuffer.Progress);
    } else {
      DstBuffer.HostVals.resize(
          std::max<size_t>(DstBuffer.HostVals.size(),
                           DstBuffer.HostBase + DstBuffer.Progress + Count));
      std::move(Vals.begin(), Vals.end(),
                DstBuffer.HostVals.begin() + DstBuffer.HostBase +
                    DstBuffer.Progress);
    }
    return {};
  }
  const bool IsMem64 = DstBuffer.Opts.isMemory64();
  EXPECTED_TRY(const uint64_t Size, getElemSize(*DstBuffer.ElemTypeInst,
                                                *DstBuffer.ElemType, IsMem64));
  if (Bytes) {
    EXPECTED_TRY(DstBuffer.Opts.Mem->setBytes(
        Raw, DstBuffer.Ptr + DstBuffer.Progress * Size, 0, Raw.size()));
    return {};
  }
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(
        checkHostValue(*DstBuffer.ElemTypeInst, *DstBuffer.ElemType, Vals[I]));
    EXPECTED_TRY(store(DstBuffer.Opts, *DstBuffer.ElemTypeInst,
                       *DstBuffer.ElemType, Vals[I],
                       DstBuffer.Ptr + (DstBuffer.Progress + I) * Size));
  }
  return {};
}

// The result of a copy built-in. See
// "include/executor/component/executor.h".
Expect<uint32_t> ComponentExecutor::waitCopyResult(
    const Runtime::Instance::ComponentInstance &Inst,
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role, bool IsAsync) {
  if (S.hasEvent(Role)) {
    return takeCopyResult(S, Role);
  }
  if (IsAsync) {
    return CopyBlocked;
  }
  Runtime::Component::Task *T = getCurrentTask(Inst);
  Runtime::Component::Thread *Cur = getCurrentThread(Inst);
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  S.setSyncWaiter(Role, true);
  const Runtime::Component::Thread::WakeReason R =
      T->collect(*Cur, [&S, Role]() { return S.hasEvent(Role); }, 0);
  if (R == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  S.setSyncWaiter(Role, false);
  return takeCopyResult(S, Role);
}

// stream.read / stream.write / future.read / future.write. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonStreamCopy(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsStream, bool IsRead) {
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const auto &Opts = Canon.getOptions();
  const bool Async = Opts.Async;
  auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint64_t Ptr = Canon.getOptions().getFlatPtr(Args[1]);
  const uint32_t Count =
      IsStream ? static_cast<uint32_t>(Canon.getOptions().getFlatPtr(Args[2]))
               : 1;
  const Runtime::Instance::Component::StreamInstance::Role Role =
      IsRead ? Runtime::Instance::Component::StreamInstance::Role::Reader
             : Runtime::Instance::Component::StreamInstance::Role::Writer;
  EXPECTED_TRY(Runtime::Instance::Component::StreamInstance * SP,
               getStream(*Inst, Idx, !IsStream, Role));
  Runtime::Instance::Component::StreamInstance &S = *SP;
  if (S.isCopying(Role)) {
    spdlog::error(ErrCode::Value::ComponentCopyBusy);
    return Unexpect(ErrCode::Value::ComponentCopyBusy);
  }
  if (S.isDone(Role)) {
    ErrCode::Value Code = ErrCode::Value::ComponentFutureWriteAfterSuccess;
    if (IsStream) {
      Code = IsRead ? ErrCode::Value::ComponentStreamReadAfterDrop
                    : ErrCode::Value::ComponentStreamWriteAfterDrop;
    } else if (IsRead) {
      Code = ErrCode::Value::ComponentFutureReadAfterSuccess;
    } else if (S.isDoneByDrop(Role)) {
      Code = ErrCode::Value::ComponentFutureWriteAfterSuccessOrDrop;
    }
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (!Async && S.getSetIdx(Role) != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (Count > MaxCopyLength) {
    spdlog::error(ErrCode::Value::ComponentStreamOpTooBig);
    return Unexpect(ErrCode::Value::ComponentStreamOpTooBig);
  }
  const auto &Elem = Canon.getValType();
  if (Count > 0 && Elem.has_value()) {
    EXPECTED_TRY(const uint32_t Align, getAlignment(*Canon.getTypeInstance(),
                                                    *Elem, Canon.isMemory64()));
    EXPECTED_TRY(const uint64_t Size, getElemSize(*Canon.getTypeInstance(),
                                                  *Elem, Canon.isMemory64()));
    EXPECTED_TRY(checkAligned(Ptr, Align));
    EXPECTED_TRY(checkInBounds(Opts, Ptr, Count * Size,
                               ErrCode::Value::MemoryOutOfBounds));
  }
  Runtime::Instance::Component::StreamBuffer Buffer;
  Buffer.Opts = Opts;
  Buffer.ElemType = Elem;
  Buffer.ElemTypeInst = Canon.getTypeInstance();
  Buffer.Ptr = Ptr;
  Buffer.Length = Count;
  S.getBuffer(Role) = std::move(Buffer);
  EXPECTED_TRY(startStreamCopy(S, Role));
  EXPECTED_TRY(const uint32_t Result, waitCopyResult(*Inst, S, Role, Async));
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
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const bool Async = Canon.getFlag();
  auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const Runtime::Instance::Component::StreamInstance::Role Role =
      IsRead ? Runtime::Instance::Component::StreamInstance::Role::Reader
             : Runtime::Instance::Component::StreamInstance::Role::Writer;
  EXPECTED_TRY(Runtime::Instance::Component::StreamInstance * SP,
               getStream(*Inst, Idx, !IsStream, Role));
  Runtime::Instance::Component::StreamInstance &S = *SP;
  if (!Async && S.getSetIdx(Role) != 0) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  if (!S.isCopying(Role) || S.isSyncWaiter(Role)) {
    const ErrCode::Value Code =
        IsRead ? ErrCode::Value::ComponentCancelReadNotPending
               : ErrCode::Value::ComponentCancelWriteNotPending;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  // The cancellation reports at once, so the result is collected here.
  S.onCancel(Role);
  Rets[0] = ValVariant(takeCopyResult(S, Role));
  return {};
}

// stream.drop-readable / stream.drop-writable / future.drop-readable /
// future.drop-writable. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonStreamDrop(const Component::CanonFunction &Canon,
                                      Span<const ValVariant> Args,
                                      bool IsStream, bool IsRead) {
  auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const Runtime::Instance::Component::StreamInstance::Role Role =
      IsRead ? Runtime::Instance::Component::StreamInstance::Role::Reader
             : Runtime::Instance::Component::StreamInstance::Role::Writer;
  EXPECTED_TRY(Runtime::Instance::Component::StreamInstance * SP,
               getStream(*Inst, Idx, !IsStream, Role));
  Runtime::Instance::Component::StreamInstance &S = *SP;
  if (S.isCopying(Role)) {
    const ErrCode::Value Code = IsRead
                                    ? ErrCode::Value::ComponentStreamRemoveBusy
                                    : ErrCode::Value::ComponentStreamDropBusy;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  // A writable future end drops only once its value or the drop of the
  // reader was delivered.
  if (!IsStream && !IsRead && !S.isDone(Role)) {
    spdlog::error(ErrCode::Value::ComponentFutureWriteEndNoValue);
    return Unexpect(ErrCode::Value::ComponentFutureWriteEndNoValue);
  }
  if (S.getSetIdx(Role) != 0) {
    EXPECTED_TRY(joinWaitableSet(*Inst, Idx, 0));
  }
  S.onDrop(Role);
  Inst->removeStreamHandle(Idx);
  return {};
}

// Check an end that forward takes. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::checkForwardEnd(
    const Component::CanonFunction &Canon,
    const Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role, uint32_t Idx,
    bool IsStream) {
  // The element type is part of the handle type.
  const auto &Elem = Canon.getValType();
  bool SameType = Elem.has_value() == S.getElemType().has_value();
  if (SameType && Elem.has_value()) {
    if (S.getElemTypeInst() == nullptr) {
      SameType = false;
    } else {
      EXPECTED_TRY(SameType,
                   matchValType(*Canon.getTypeInstance(), *Elem,
                                *S.getElemTypeInst(), *S.getElemType()));
    }
  }
  if (!SameType) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    handle index {} used with the wrong type"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (S.isCopying(Role)) {
    const ErrCode::Value Code =
        IsStream ? ErrCode::Value::ComponentStreamForwardBusy
                 : ErrCode::Value::ComponentFutureForwardBusy;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (S.isDone(Role)) {
    ErrCode::Value Code = ErrCode::Value::ComponentFutureForwardAfterDone;
    if (IsStream) {
      Code = Role == Runtime::Instance::Component::StreamInstance::Role::Reader
                 ? ErrCode::Value::ComponentStreamForwardAfterWriterDrop
                 : ErrCode::Value::ComponentStreamForwardAfterReaderDrop;
    }
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (S.getSetIdx(Role) != 0) {
    const ErrCode::Value Code =
        IsStream ? ErrCode::Value::ComponentStreamForwardInSet
                 : ErrCode::Value::ComponentFutureForwardInSet;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  return {};
}

// stream.forward / future.forward. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonStreamForward(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args,
                                         bool IsStream) {
  auto *Inst = Canon.getInstance();
  const uint32_t ReadIdx = Args[0].get<uint32_t>();
  const uint32_t WriteIdx = Args[1].get<uint32_t>();
  // The readable end leaves the table first, so the writable end cannot
  // name the same handle.
  EXPECTED_TRY(
      Runtime::Instance::Component::StreamInstance * Src,
      getStream(*Inst, ReadIdx, !IsStream,
                Runtime::Instance::Component::StreamInstance::Role::Reader));
  EXPECTED_TRY(checkForwardEnd(
      Canon, *Src, Runtime::Instance::Component::StreamInstance::Role::Reader,
      ReadIdx, IsStream));
  Inst->removeStreamHandle(ReadIdx);
  EXPECTED_TRY(
      Runtime::Instance::Component::StreamInstance * Dst,
      getStream(*Inst, WriteIdx, !IsStream,
                Runtime::Instance::Component::StreamInstance::Role::Writer));
  EXPECTED_TRY(checkForwardEnd(
      Canon, *Dst, Runtime::Instance::Component::StreamInstance::Role::Writer,
      WriteIdx, IsStream));
  Inst->removeStreamHandle(WriteIdx);
  // When both joined peers wait, the smaller copy runs again.
  const auto [Again, AgainRole] = Src->onForward(*Dst);
  if (Again != nullptr) {
    EXPECTED_TRY(startStreamCopy(*Again, AgainRole));
  }
  return {};
}

// error-context.new. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonErrorContextNew(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets) {
  const uint64_t Ptr = Canon.getOptions().getFlatPtr(Args[0]);
  const uint64_t Len = Canon.getOptions().getFlatPtr(Args[1]);
  EXPECTED_TRY(auto Msg, loadString(Canon.getOptions(), Ptr, Len));
  Rets[0] = ValVariant(Canon.getInstance()->addErrorContext(std::move(Msg)));
  return {};
}

// error-context.debug-message. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonErrorContextDebugMessage(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  const auto &Opts = Canon.getOptions();
  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint64_t Ptr = Canon.getOptions().getFlatPtr(Args[1]);
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
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  auto *Inst = Canon.getInstance();
  const uint32_t SetIdx = Args[0].get<uint32_t>();
  const uint64_t Ptr = Canon.getOptions().getFlatPtr(Args[1]);
  auto *Set = Inst->findWaitableSet(SetIdx);
  if (Set == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto FirstPending = [this, Inst,
                       SetIdx]() -> std::optional<Component::WaitableEvent> {
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
    EXPECTED_TRY(const Component::WaitableEvent Ev,
                 waitOnWaitableSet(*T, *Cur, *Inst, SetIdx, false));
    EXPECTED_TRY(const uint32_t Code, storeEvent(Canon, Ev, Ptr));
    Rets[0] = ValVariant(Code);
    return {};
  }
  // A poll reports what is pending without waiting; the event memory is
  // written even when there is no event.
  std::optional<Component::WaitableEvent> Ev = FirstPending();
  if (!Ev.has_value()) {
    Ev = Component::WaitableEvent{Component::EventCode::None, 0, 0};
  }
  EXPECTED_TRY(const uint32_t Code, storeEvent(Canon, *Ev, Ptr));
  Rets[0] = ValVariant(Code);
  return {};
}

// waitable-set.drop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonWaitableSetDrop(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args) {
  auto *Inst = Canon.getInstance();
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
ComponentExecutor::runCanonThreadIndex(const Component::CanonFunction &Canon,
                                       Span<ValVariant> Rets) {
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
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
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
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
  auto *Inst = T->getInstance();
  Runtime::Component::Thread *Cur =
      newRootThread(*Inst->getRoot(), *T, [this, T, Inst, Func, Context]() {
        Runtime::Component::Thread *Own = getCurrentThread(*Inst);
        assuming(Own != nullptr);
        auto Res =
            invokeCore(Func, std::vector<ValVariant>{ValVariant(Context)});
        if (Res) {
          auto Left = T->removeThread(*Own);
          if (!Left) {
            setTrap(Left.error(), *Inst);
          }
          return;
        }
        if (Res.error() != ErrCode::Value::ComponentAsyncAborted) {
          setTrap(Res.error(), *Inst);
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
  Canon.getInstance()->getRoot()->pushWaiting(*Other);
  return {};
}

// thread.suspend. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonThreadSuspend(const Component::CanonFunction &Canon,
                                         Span<ValVariant> Rets) {
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
  if (T == nullptr || Cur == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  if (T->suspend(*Cur) == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(UINT32_C(0));
  return {};
}

// The four thread.{suspend,yield}-then-{resume,promote} built-ins. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanonThreadSwitch(
    const Component::CanonFunction &Canon, Span<const ValVariant> Args,
    Span<ValVariant> Rets, bool IsYield, bool IsPromote) {
  Runtime::Component::Task *T = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *Cur = getCurrentThread(*Canon.getInstance());
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
  Runtime::Component::Thread::WakeReason R;
  if (IsPromote) {
    // A thread cannot promote itself; otherwise the named thread takes the
    // stack only if it is ready, and this one parks.
    if (Other == Cur) {
      spdlog::error(ErrCode::Value::ComponentThreadNotSuspended);
      return Unexpect(ErrCode::Value::ComponentThreadNotSuspended);
    }
    R = T->promote(*Cur, *Other, IsYield);
  } else {
    // The named thread takes the stack next.
    if (!Other->isSuspended()) {
      spdlog::error(ErrCode::Value::ComponentThreadNotSuspended);
      return Unexpect(ErrCode::Value::ComponentThreadNotSuspended);
    }
    R = T->switchTo(*Cur, *Other, IsYield);
  }
  if (R == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Rets[0] = ValVariant(UINT32_C(0));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
