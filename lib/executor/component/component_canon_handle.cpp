// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/component/taskmgr.h"

#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

Expect<uint64_t> LiftLowerContext::liftHandle(uint32_t TypeIdx, uint32_t Idx,
                                              bool Own) const noexcept {
  const auto *RT = getTypeResource(TypeIdx);
  if (getInstance() == nullptr || RT == nullptr) {
    // No table context (unit ABI tests): pass the raw value through.
    return Idx;
  }
  if (!Own && RT->getImpl() == getInstance()) {
    // The owning instance passes representations directly for borrows.
    return Idx;
  }
  auto *Slot = getInstance()->getHandle(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (Own) {
    if (!Slot->Own || Slot->Lends != 0) {
      spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
      spdlog::error("    handle index {}"sv, Idx);
      return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
    }
    // Transferring ownership out of the instance removes the handle.
    return getInstance()->removeHandle(Idx)->Rep;
  }
  // The borrow lends the handle for the duration of the call.
  Slot->Lends += 1;
  if (getLiftedBorrows() != nullptr) {
    getLiftedBorrows()->emplace_back(getInstance(), Idx);
  }
  return Slot->Rep;
}

uint32_t LiftLowerContext::lowerHandle(uint32_t TypeIdx, uint64_t Rep,
                                       bool Own) const noexcept {
  const auto *RT = getTypeResource(TypeIdx);
  if (getInstance() == nullptr || RT == nullptr) {
    return Rep;
  }
  // Borrows lowered into the owning instance get the representation.
  if (!Own && RT->getImpl() == getInstance()) {
    return Rep;
  }
  if (!Own && getBorrowTask() != nullptr) {
    // The receiving task scopes the borrow.
    getBorrowTask()->NumBorrows += 1;
    return getInstance()->addHandle(RT, Rep, Own, getBorrowTask());
  }
  return getInstance()->addHandle(RT, Rep, Own);
}

// Transferring a readable end removes it from the sender's table.
Expect<std::shared_ptr<void>>
LiftLowerContext::liftTransmitEnd(bool IsStream, uint32_t Idx) const noexcept {
  const auto WantKind =
      IsStream ? Runtime::Instance::Component::WaitableBase::Kind::StreamRead
               : Runtime::Instance::Component::WaitableBase::Kind::FutureRead;
  auto *W =
      getInstance() != nullptr ? getInstance()->getWaitable(Idx) : nullptr;
  if (W == nullptr || W->getKind() != WantKind) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto *E = static_cast<Runtime::Instance::Component::TransmitEnd *>(W);
  if (E->Status == Runtime::Instance::Component::TransmitEnd::State::Done &&
      E->DoneByDrop) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (E->isInWaitableSet()) {
    const auto Code = IsStream ? ErrCode::Value::ComponentStreamLiftInSet
                               : ErrCode::Value::ComponentFutureLiftInSet;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (E->Status == Runtime::Instance::Component::TransmitEnd::State::Done) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    return Unexpect(Code);
  }
  if (E->isCopying()) {
    spdlog::error(ErrCode::Value::ComponentStreamRemoveBusy);
    return Unexpect(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  auto Shared = E->Shared;
  getInstance()->removeWaitable(Idx);
  return std::static_pointer_cast<void>(Shared);
}

// Entering an instance mints a fresh readable end over the shared object.
Expect<uint32_t> LiftLowerContext::lowerTransmitEnd(
    bool IsStream, const std::shared_ptr<void> &SharedV) const noexcept {
  if (getInstance() == nullptr || !SharedV) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    cannot lower a stream or future without a table"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  auto Shared =
      std::static_pointer_cast<Runtime::Instance::Component::TransmitState>(
          SharedV);
  auto End = std::make_shared<Runtime::Instance::Component::TransmitEnd>(
      IsStream ? Runtime::Instance::Component::WaitableBase::Kind::StreamRead
               : Runtime::Instance::Component::WaitableBase::Kind::FutureRead,
      Shared);
  auto *EndP = End.get();
  const uint32_t Idx = getInstance()->addWaitable(std::move(End));
  EndP->TableIdx = Idx;
  return Idx;
}

// Invoke the guest's realloc; a failed invoke traps.
Expect<uint64_t>
LiftLowerContext::callRealloc(uint64_t OldPtr, uint64_t OldSize, uint32_t Align,
                              uint64_t NewSize) const noexcept {
  if (getExecutor() == nullptr || getRealloc() == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    canonical ABI: realloc required but not provided"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  // realloc takes the address type of the memory it serves.
  std::array<ValVariant, 4> Args{lowerPtr(OldPtr), lowerPtr(OldSize),
                                 lowerPtr(Align), lowerPtr(NewSize)};
  auto ParamTypes = getRealloc()->getFuncType().getParamTypes();
  EXPECTED_TRY(auto Res, getExecutor()->getCoreExecutor().invoke(
                             getRealloc(), Args, ParamTypes));
  if (Res.empty()) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    canonical ABI: realloc returned no value"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const uint64_t Ptr = liftPtr(Res[0].first);
  // The pointer is alignment-checked before bounds; 0 is valid.
  if (Align > 1U && (Ptr & static_cast<uint64_t>(Align - 1U)) != 0U) {
    spdlog::error(ErrCode::Value::ComponentPtrUnaligned);
    spdlog::error("    canonical ABI: realloc return: result not aligned"sv);
    return Unexpect(ErrCode::Value::ComponentPtrUnaligned);
  }
  if (getMemory() != nullptr) {
    const uint64_t End = Ptr + NewSize;
    if (End > getMemory()->getPageSize() *
                  Runtime::Instance::MemoryInstance::kPageSize) {
      spdlog::error(ErrCode::Value::ComponentReallocOOB);
      spdlog::error(ErrInfo::InfoBoundary(
          Ptr, NewSize,
          getMemory()->getPageSize() *
              Runtime::Instance::MemoryInstance::kPageSize));
      return Unexpect(ErrCode::Value::ComponentReallocOOB);
    }
  }
  return Ptr;
}

// An error-context travels as an index into the lifter's handles.
Expect<ComponentValVariant>
LiftLowerContext::liftErrorContext(uint32_t Idx) const noexcept {
  const auto *Obj =
      getInstance() != nullptr ? getInstance()->getErrorContext(Idx) : nullptr;
  if (Obj == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    handle {} is not an error-context"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  return makeComponentVal(ErrorContextVal{*Obj});
}

Expect<uint32_t> LiftLowerContext::lowerErrorContext(
    const ComponentValVariant &V) const noexcept {
  if (!isComponentVal<ErrorContextVal>(V) || getInstance() == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    expected an error-context value"sv);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  return getInstance()->addErrorContext(
      getComponentVal<ErrorContextVal>(V).Message);
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
