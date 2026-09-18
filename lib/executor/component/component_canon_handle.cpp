// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
ErrCode logUnknownHandle(uint32_t Idx) noexcept {
  spdlog::error(ErrCode::Value::ComponentHandleUnknown);
  spdlog::error("    unknown handle index {}"sv, Idx);
  return ErrCode::Value::ComponentHandleUnknown;
}

ErrCode logWrongType(uint32_t Idx) noexcept {
  spdlog::error(ErrCode::Value::ComponentHandleWrongType);
  spdlog::error("    handle index {} used with the wrong type"sv, Idx);
  return ErrCode::Value::ComponentHandleWrongType;
}

Expect<const Runtime::Instance::Component::ResourceTypeInstance *>
getResourceType(const Runtime::Instance::ComponentInstance &TypeInst,
                uint32_t TypeIdx) {
  EXPECTED_TRY(const auto *ResType, TypeInst.getTypeResource(TypeIdx));
  if (ResType == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ResourceType));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  return ResType;
}
} // namespace

// A resource handle of the table. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::ComponentInstance::ResourceHandle *>
ComponentExecutor::getHandle(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
    const Runtime::Instance::Component::ResourceTypeInstance *ResType) {
  if (!Inst.getHandleSlot(Idx)) {
    return Unexpect(logUnknownHandle(Idx));
  }
  auto *H = Inst.findHandle(Idx);
  if (H == nullptr) {
    return Unexpect(logWrongType(Idx));
  }
  if (ResType != nullptr && H->ResType != ResType) {
    return Unexpect(logWrongType(Idx));
  }
  return H;
}

// A subtask of the table. See "include/executor/component/executor.h".
Expect<Runtime::Component::Task *>
ComponentExecutor::getSubtask(const Runtime::Instance::ComponentInstance &Inst,
                              uint32_t Idx) {
  if (!Inst.getHandleSlot(Idx)) {
    return Unexpect(logUnknownHandle(Idx));
  }
  Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
  if (Callee == nullptr) {
    return Unexpect(logWrongType(Idx));
  }
  return Callee;
}

// A stream or future end of the table. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::Stream *> ComponentExecutor::getStreamEnd(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
    bool IsFuture, Runtime::Instance::Component::Stream::End E) {
  if (!Inst.getHandleSlot(Idx)) {
    return Unexpect(logUnknownHandle(Idx));
  }
  const auto *Slot = Inst.findStreamEnd(Idx);
  if (Slot == nullptr) {
    return Unexpect(logWrongType(Idx));
  }
  if (Slot->first->isFuture() != IsFuture || Slot->second != E) {
    return Unexpect(logWrongType(Idx));
  }
  return Slot->first.get();
}

// Whether a waitable has an event pending. See
// "include/executor/component/executor.h".
bool ComponentExecutor::hasPendingEvent(
    const Runtime::Instance::ComponentInstance &Inst,
    uint32_t Idx) const noexcept {
  if (const Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
      Callee != nullptr) {
    return Callee->getSubtaskCode() != Callee->getDeliveredCode();
  }
  if (const auto *Slot = Inst.findStreamEnd(Idx); Slot != nullptr) {
    return Slot->first->hasPending(Slot->second);
  }
  return false;
}

// Collect the event of a waitable. See
// "include/executor/component/executor.h".
ComponentExecutor::Event ComponentExecutor::takePendingEvent(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx) noexcept {
  if (Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
      Callee != nullptr) {
    // The progress of the callee since the caller last looked; a resolution
    // delivered releases what the call borrowed.
    const uint32_t Code = Callee->getSubtaskCode();
    Callee->setDeliveredCode(Code);
    if (Callee->isResolved() && !Callee->isDelivered()) {
      Callee->setDelivered();
      Callee->releaseLenders();
    }
    return Event{EventCode::Subtask, Idx, Code};
  }
  const auto *Slot = Inst.findStreamEnd(Idx);
  assuming(Slot != nullptr && Slot->first->hasPending(Slot->second));
  Runtime::Instance::Component::Stream &S = *Slot->first;
  EventCode Code = EventCode::StreamWrite;
  if (Slot->second == Runtime::Instance::Component::Stream::End::Readable) {
    Code = S.isFuture() ? EventCode::FutureRead : EventCode::StreamRead;
  } else if (S.isFuture()) {
    Code = EventCode::FutureWrite;
  }
  return Event{Code, Idx, takeOutcome(S, Slot->second)};
}

// The payload of a collected copy. See
// "include/executor/component/executor.h".
uint32_t ComponentExecutor::takeOutcome(
    Runtime::Instance::Component::Stream &S,
    Runtime::Instance::Component::Stream::End E) noexcept {
  const auto [Res, Count] = S.onCollect(E);
  const auto Code = static_cast<uint32_t>(Res);
  return S.isFuture() ? Code : (Code | (Count << 4));
}

// Move a waitable between sets. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::joinWaitableSet(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
    uint32_t SetIdx) {
  Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
  const auto *Slot = Inst.findStreamEnd(Idx);
  bool SyncWaiter = false;
  uint32_t Joined = 0;
  if (Callee != nullptr) {
    SyncWaiter = Callee->isSyncWaiter();
    Joined = Callee->getSetIdx();
  } else if (Slot != nullptr) {
    SyncWaiter = Slot->first->isSyncWaiter(Slot->second);
    Joined = Slot->first->getSetIdx(Slot->second);
  } else if (Inst.getHandleSlot(Idx)) {
    return Unexpect(logWrongType(Idx));
  } else {
    return Unexpect(logUnknownHandle(Idx));
  }
  if (SyncWaiter) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  std::vector<uint32_t> *Members = nullptr;
  if (SetIdx != 0) {
    if (!Inst.getHandleSlot(SetIdx)) {
      return Unexpect(logUnknownHandle(SetIdx));
    }
    Members = Inst.findWaitableSet(SetIdx);
    if (Members == nullptr) {
      return Unexpect(logWrongType(SetIdx));
    }
  }
  if (Joined != 0) {
    if (auto *Old = Inst.findWaitableSet(Joined); Old != nullptr) {
      Old->erase(std::remove(Old->begin(), Old->end(), Idx), Old->end());
    }
  }
  if (Callee != nullptr) {
    Callee->setSetIdx(SetIdx);
  } else {
    Slot->first->setSetIdx(Slot->second, SetIdx);
  }
  if (Members != nullptr) {
    Members->push_back(Idx);
  }
  return {};
}

// Lower an own handle. See "include/executor/component/executor.h".
Expect<uint32_t> ComponentExecutor::lowerOwn(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    uint64_t Rep) {
  EXPECTED_TRY(const auto *ResType, getResourceType(TypeInst, TypeIdx));
  return Opts.Inst->addHandle(ResType, Rep, /*Own=*/true);
}

// Lower a borrow handle. See "include/executor/component/executor.h".
Expect<ValVariant> ComponentExecutor::lowerBorrow(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    uint64_t Rep) {
  EXPECTED_TRY(const auto *ResType, getResourceType(TypeInst, TypeIdx));
  // The implementing instance gets the representation itself.
  if (Opts.Inst == ResType->getImpl()) {
    return ValVariant(static_cast<uint32_t>(Rep));
  }
  Runtime::Component::Task *T = getCurrentTask();
  const uint32_t Idx = Opts.Inst->addHandle(ResType, Rep, /*Own=*/false, T);
  if (T != nullptr) {
    T->addBorrow();
  }
  return ValVariant(Idx);
}

// Lift an own handle. See "include/executor/component/executor.h".
Expect<uint64_t>
ComponentExecutor::liftOwn(const Runtime::Component::CanonOptions &Opts,
                           const Runtime::Instance::ComponentInstance &TypeInst,
                           uint32_t TypeIdx, uint32_t Handle) {
  EXPECTED_TRY(const auto *ResType, getResourceType(TypeInst, TypeIdx));
  EXPECTED_TRY(auto *H, getHandle(*Opts.Inst, Handle, ResType));
  if (H->NumLends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  if (!H->Own) {
    return Unexpect(logWrongType(Handle));
  }
  const uint64_t Rep = H->Rep;
  Opts.Inst->removeHandle(Handle);
  return Rep;
}

// Lift a borrow handle. See "include/executor/component/executor.h".
Expect<uint64_t> ComponentExecutor::liftBorrow(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    uint32_t Handle) {
  EXPECTED_TRY(const auto *ResType, getResourceType(TypeInst, TypeIdx));
  EXPECTED_TRY(auto *H, getHandle(*Opts.Inst, Handle, ResType));
  // The callee task borrows it until its resolution is delivered.
  if (Runtime::Component::Task *T = getCurrentTask(); T != nullptr) {
    T->addLender(H);
  }
  return H->Rep;
}

// Lower a stream or future. See "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::lowerStream(const Runtime::Component::CanonOptions &Opts,
                               const ComponentValVariant &Val) {
  const auto &Shared = getComponentVal<StreamFutureVal>(Val).Shared;
  // The readable end enters the table of the receiving instance.
  return Opts.Inst->addStreamEnd(
      Runtime::Instance::Component::Stream::from(Shared),
      Runtime::Instance::Component::Stream::End::Readable);
}

// Lift a stream or future. See "include/executor/component/executor.h".
Expect<ComponentValVariant>
ComponentExecutor::liftStream(const Runtime::Component::CanonOptions &Opts,
                              uint32_t Handle, bool IsStream) {
  const auto E = Runtime::Instance::Component::Stream::End::Readable;
  EXPECTED_TRY(Runtime::Instance::Component::Stream * S,
               getStreamEnd(*Opts.Inst, Handle, !IsStream, E));
  // A readable end leaves its table only when nothing is in flight on it.
  if (S->isDone(E)) {
    if (IsStream) {
      spdlog::error(ErrCode::Value::ComponentStreamLiftAfterDrop);
      spdlog::error("    cannot lift stream after being notified that the "
                    "writable end dropped"sv);
      return Unexpect(ErrCode::Value::ComponentStreamLiftAfterDrop);
    }
    spdlog::error(ErrCode::Value::ComponentFutureLiftAfterSuccess);
    return Unexpect(ErrCode::Value::ComponentFutureLiftAfterSuccess);
  }
  if (S->getSetIdx(E) != 0) {
    if (IsStream) {
      spdlog::error(ErrCode::Value::ComponentStreamLiftInSet);
      return Unexpect(ErrCode::Value::ComponentStreamLiftInSet);
    }
    spdlog::error(ErrCode::Value::ComponentFutureLiftInSet);
    return Unexpect(ErrCode::Value::ComponentFutureLiftInSet);
  }
  if (S->isBusy(E)) {
    spdlog::error(ErrCode::Value::ComponentStreamRemoveBusy);
    return Unexpect(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  auto Shared = Opts.Inst->removeStreamEnd(Handle);
  return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
}

} // namespace Executor
} // namespace WasmEdge
