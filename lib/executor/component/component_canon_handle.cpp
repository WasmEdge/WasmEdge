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
auto logUnknownHandle(uint32_t Idx) {
  spdlog::error(ErrCode::Value::ComponentHandleUnknown);
  spdlog::error("    unknown handle index {}"sv, Idx);
  return Unexpect(ErrCode::Value::ComponentHandleUnknown);
}

auto logWrongHandleType(uint32_t Idx) {
  spdlog::error(ErrCode::Value::ComponentHandleWrongType);
  spdlog::error("    handle index {} used with the wrong type"sv, Idx);
  return Unexpect(ErrCode::Value::ComponentHandleWrongType);
}

Expect<const Runtime::Instance::Component::ResourceTypeInstance *>
getResourceTypeByIdx(const Runtime::Instance::ComponentInstance &TypeInst,
                     uint32_t TypeIdx) {
  EXPECTED_TRY(const auto *ResType, TypeInst.getTypeResource(TypeIdx));
  if (ResType == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ResourceType));
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  return ResType;
}
} // namespace

// A resource handle of the table. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::ResourceHandle *>
ComponentExecutor::getResourceHandle(
    Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
    const Runtime::Instance::Component::ResourceTypeInstance *ResType) {
  if (!Inst.getHandleSlot(Idx)) {
    return logUnknownHandle(Idx);
  }
  auto *H = Inst.findResourceHandle(Idx);
  if (H == nullptr) {
    return logWrongHandleType(Idx);
  }
  if (ResType != nullptr && H->ResType != ResType) {
    return logWrongHandleType(Idx);
  }
  return H;
}

// A subtask of the table. See "include/executor/component/executor.h".
Expect<Runtime::Component::Task *>
ComponentExecutor::getSubtask(const Runtime::Instance::ComponentInstance &Inst,
                              uint32_t Idx) {
  if (!Inst.getHandleSlot(Idx)) {
    return logUnknownHandle(Idx);
  }
  Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
  if (Callee == nullptr) {
    return logWrongHandleType(Idx);
  }
  return Callee;
}

// The stream behind a stream or future handle. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance *>
ComponentExecutor::getStream(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
    bool IsFuture, Runtime::Instance::Component::StreamInstance::Role Role) {
  if (!Inst.getHandleSlot(Idx)) {
    return logUnknownHandle(Idx);
  }
  const auto *Slot = Inst.findStreamHandle(Idx);
  if (Slot == nullptr) {
    return logWrongHandleType(Idx);
  }
  if (Slot->Stream->isFuture() != IsFuture || Slot->Role != Role) {
    return logWrongHandleType(Idx);
  }
  return Slot->Stream;
}

// Whether a waitable has an event pending. See
// "include/executor/component/executor.h".
bool ComponentExecutor::hasPendingEvent(
    const Runtime::Instance::ComponentInstance &Inst,
    uint32_t Idx) const noexcept {
  if (const Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
      Callee != nullptr) {
    return Callee->hasPendingEvent();
  }
  if (const auto *Slot = Inst.findStreamHandle(Idx); Slot != nullptr) {
    return Slot->Stream->hasEvent(Slot->Role);
  }
  return false;
}

// Collect the event of a waitable. See
// "include/executor/component/executor.h".
Component::WaitableEvent ComponentExecutor::takePendingEvent(
    const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx) noexcept {
  if (Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
      Callee != nullptr) {
    return Component::WaitableEvent{Component::EventCode::Subtask, Idx,
                                    Callee->onCollect()};
  }
  const auto *Slot = Inst.findStreamHandle(Idx);
  assuming(Slot != nullptr);
  Runtime::Instance::Component::StreamInstance &S = *Slot->Stream;
  assuming(S.hasEvent(Slot->Role));
  Component::EventCode Code = Component::EventCode::StreamWrite;
  if (Slot->Role ==
      Runtime::Instance::Component::StreamInstance::Role::Reader) {
    Code = S.isFuture() ? Component::EventCode::FutureRead
                        : Component::EventCode::StreamRead;
  } else if (S.isFuture()) {
    Code = Component::EventCode::FutureWrite;
  }
  return Component::WaitableEvent{Code, Idx, takeCopyResult(S, Slot->Role)};
}

// The payload of a collected copy. See
// "include/executor/component/executor.h".
uint32_t ComponentExecutor::takeCopyResult(
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role) noexcept {
  const auto [Res, Count] = S.onCollect(Role);
  const auto Code = static_cast<uint32_t>(Res);
  return S.isFuture() ? Code : (Code | (Count << 4));
}

// Move a waitable between sets. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::joinWaitableSet(Runtime::Instance::ComponentInstance &Inst,
                                   uint32_t Idx, uint32_t SetIdx) {
  Runtime::Component::Task *Callee = Inst.findSubtask(Idx);
  const auto *Slot = Inst.findStreamHandle(Idx);
  bool SyncWaiter = false;
  uint32_t Joined = 0;
  if (Callee != nullptr) {
    SyncWaiter = Callee->isSyncWaiter();
    Joined = Callee->getSetIdx();
  } else if (Slot != nullptr) {
    SyncWaiter = Slot->Stream->isSyncWaiter(Slot->Role);
    Joined = Slot->Stream->getSetIdx(Slot->Role);
  } else if (Inst.getHandleSlot(Idx)) {
    return logWrongHandleType(Idx);
  } else {
    return logUnknownHandle(Idx);
  }
  if (SyncWaiter) {
    spdlog::error(ErrCode::Value::ComponentWaitableInSetSyncUse);
    return Unexpect(ErrCode::Value::ComponentWaitableInSetSyncUse);
  }
  std::vector<uint32_t> *Members = nullptr;
  if (SetIdx != 0) {
    if (!Inst.getHandleSlot(SetIdx)) {
      return logUnknownHandle(SetIdx);
    }
    Members = Inst.findWaitableSet(SetIdx);
    if (Members == nullptr) {
      return logWrongHandleType(SetIdx);
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
    Slot->Stream->setSetIdx(Slot->Role, SetIdx);
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
  EXPECTED_TRY(const auto *ResType, getResourceTypeByIdx(TypeInst, TypeIdx));
  return Opts.Inst->addResourceHandle(ResType, Rep, /*Own=*/true);
}

// Lower a borrow handle. See "include/executor/component/executor.h".
Expect<ValVariant> ComponentExecutor::lowerBorrow(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    uint64_t Rep) {
  EXPECTED_TRY(const auto *ResType, getResourceTypeByIdx(TypeInst, TypeIdx));
  // The implementing instance gets the representation itself.
  if (Opts.Inst == ResType->getImpl()) {
    return ValVariant(static_cast<uint32_t>(Rep));
  }
  Runtime::Component::Task *T = getCurrentTask(*Opts.Inst);
  const uint32_t Idx =
      Opts.Inst->addResourceHandle(ResType, Rep, /*Own=*/false, T);
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
  EXPECTED_TRY(const auto *ResType, getResourceTypeByIdx(TypeInst, TypeIdx));
  EXPECTED_TRY(auto *H, getResourceHandle(*Opts.Inst, Handle, ResType));
  if (H->NumLends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  if (!H->Own) {
    return logWrongHandleType(Handle);
  }
  const uint64_t Rep = H->Rep;
  Opts.Inst->removeResourceHandle(Handle);
  return Rep;
}

// Lift a borrow handle. See "include/executor/component/executor.h".
Expect<uint64_t> ComponentExecutor::liftBorrow(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    uint32_t Handle) {
  EXPECTED_TRY(const auto *ResType, getResourceTypeByIdx(TypeInst, TypeIdx));
  EXPECTED_TRY(auto *H, getResourceHandle(*Opts.Inst, Handle, ResType));
  // The callee task borrows it until its resolution is delivered.
  if (Runtime::Component::Task *T = getCurrentTask(*Opts.Inst); T != nullptr) {
    T->addLender(H);
  }
  return H->Rep;
}

// Lower a stream or future. See "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::lowerStream(const Runtime::Component::CanonOptions &Opts,
                               const ComponentValVariant &Val) {
  // The reader handle enters the table of the receiving instance.
  const auto &End = getComponentVal<StreamFutureVal>(Val);
  auto *S = End.Stream;
  if (S == nullptr) {
    // A value of the embedder names the end by its handle in the table of
    // the root it enters, where the result that carried it left it.
    auto *Root = Opts.Inst->getRoot();
    const auto *Slot = Root->findStreamHandle(End.HandleIdx);
    if (Slot == nullptr ||
        Slot->Role !=
            Runtime::Instance::Component::StreamInstance::Role::Reader ||
        Slot->Stream->isFuture() == End.IsStream) {
      return logUnknownHandle(End.HandleIdx);
    }
    S = Root->removeStreamHandle(End.HandleIdx);
  }
  return Opts.Inst->addStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Reader);
}

// Lift a stream or future. See "include/executor/component/executor.h".
Expect<ComponentValVariant>
ComponentExecutor::liftStream(const Runtime::Component::CanonOptions &Opts,
                              uint32_t Handle, bool IsStream) {
  const auto Role = Runtime::Instance::Component::StreamInstance::Role::Reader;
  EXPECTED_TRY(Runtime::Instance::Component::StreamInstance * S,
               getStream(*Opts.Inst, Handle, !IsStream, Role));
  // A reader handle leaves its table only when nothing is in flight on it.
  if (S->isDone(Role)) {
    if (IsStream) {
      spdlog::error(ErrCode::Value::ComponentStreamLiftAfterDrop);
      spdlog::error("    cannot lift stream after being notified that the "
                    "writable end dropped"sv);
      return Unexpect(ErrCode::Value::ComponentStreamLiftAfterDrop);
    }
    spdlog::error(ErrCode::Value::ComponentFutureLiftAfterSuccess);
    return Unexpect(ErrCode::Value::ComponentFutureLiftAfterSuccess);
  }
  if (S->getSetIdx(Role) != 0) {
    if (IsStream) {
      spdlog::error(ErrCode::Value::ComponentStreamLiftInSet);
      return Unexpect(ErrCode::Value::ComponentStreamLiftInSet);
    }
    spdlog::error(ErrCode::Value::ComponentFutureLiftInSet);
    return Unexpect(ErrCode::Value::ComponentFutureLiftInSet);
  }
  if (S->isCopying(Role)) {
    spdlog::error(ErrCode::Value::ComponentStreamRemoveBusy);
    return Unexpect(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  return makeComponentVal(
      StreamFutureVal{Opts.Inst->removeStreamHandle(Handle), 0, IsStream});
}

} // namespace Executor
} // namespace WasmEdge
