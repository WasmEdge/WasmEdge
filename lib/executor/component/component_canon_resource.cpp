// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// resource.new. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonResourceNew(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args,
                                       Span<ValVariant> Rets) {
  const auto *ResType = Canon.getResource();
  const uint64_t Rep = ResType->getRep(Args[0]);
  Rets[0] = ValVariant(
      Canon.getInstance()->addResourceHandle(ResType, Rep, /*Own=*/true));
  return {};
}

// resource.rep. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonResourceRep(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args,
                                       Span<ValVariant> Rets) {
  const auto *ResType = Canon.getResource();
  EXPECTED_TRY(const auto *H,
               getResourceHandle(*Canon.getInstance(), Args[0].get<uint32_t>(),
                                 ResType));
  Rets[0] = ResType->getRepSlot(H->Rep);
  return {};
}

// resource.drop. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonResourceDrop(const Component::CanonFunction &Canon,
                                        Span<const ValVariant> Args) {
  const auto *ResType = Canon.getResource();
  auto *Inst = Canon.getInstance();
  const uint32_t Idx = Args[0].get<uint32_t>();
  EXPECTED_TRY(const auto *H, getResourceHandle(*Inst, Idx, ResType));
  if (H->NumLends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  const auto Handle = *H;
  Inst->removeResourceHandle(Idx);
  if (!Handle.Own) {
    // A borrow ends: the task that received it owes one fewer.
    if (Handle.BorrowScope != nullptr) {
      Handle.BorrowScope->dropBorrow();
    }
    return {};
  }
  // An own handle runs the destructor in the implementing instance, which
  // must not have trapped.
  auto *Impl = ResType->getImpl();
  if (Impl != Inst && Impl->getRoot()->isPoisoned()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  if (ResType->getDtor() != nullptr) {
    return runResourceDtor(Impl, getCurrentThread(*Inst), ResType->getDtor(),
                           Handle.Rep);
  }
  if (ResType->getHostDtor()) {
    ResType->getHostDtor()(Handle.Rep);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
