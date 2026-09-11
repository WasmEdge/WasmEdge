// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errcode.h"
#include "common/spdlog.h"
#include "runtime/component/task.h"
#include "runtime/component/taskmgr.h"

#include <cstdint>
#include <functional>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Resume ready parked threads until Done(). See executor.h.
Expect<void>
ComponentExecutor::pumpUntil(const std::function<bool()> &Done,
                             Runtime::Component::TaskThread *Root) noexcept {
  auto &Reactor = TaskMgr.getReactor();
  while (true) {
    EXPECTED_TRY(drainPostedTransmits());
    if (Done()) {
      break;
    }
    if (TaskMgr.getTrapLatch().has_value()) {
      return Unexpect(*TaskMgr.getTrapLatch());
    }
    // A non-async-typed call parked on the root thread restricts what may
    // take the stack.
    const Runtime::Component::Task *Restrict =
        Root != nullptr ? Root->Pin : nullptr;
    const uint64_t Seen = Reactor.getGeneration();
    // The pinned call itself continues as soon as it can; only then may
    // another thread run under it.
    Runtime::Component::TaskThread *Pick = nullptr;
    if (Restrict != nullptr && Root->ReadyFn && Root->ReadyFn() &&
        TaskMgr.takeParkedThread(Root)) {
      Pick = Root;
    } else {
      Pick = TaskMgr.takeReadyThread(
          [this, Restrict](const Runtime::Component::TaskThread &Thread) {
            return Restrict == nullptr || canResumeUnder(*Restrict, Thread);
          });
    }
    if (Pick == nullptr) {
      // A host operation in flight can still make a parked thread ready.
      if (Reactor.wait(Seen)) {
        continue;
      }
      spdlog::error(ErrCode::Value::ComponentAsyncDeadlock);
      spdlog::error(
          "    deadlock detected: event loop cannot make further progress"sv);
      return Unexpect(ErrCode::Value::ComponentAsyncDeadlock);
    }
    TaskMgr.resumeThread(Pick, Runtime::Component::ResumeReason::Normal);
    if (TaskMgr.getTrapLatch().has_value()) {
      return Unexpect(*TaskMgr.getTrapLatch());
    }
  }
  return {};
}

// Complete the posted host rendezvous. See executor.h.
Expect<void> ComponentExecutor::drainPostedTransmits() noexcept {
  for (auto &S : TaskMgr.takePostedTransmits()) {
    if (auto *E = S->PostedEnd; E != nullptr) {
      S->PostedEnd = nullptr;
      EXPECTED_TRY(startCopy(*E));
    }
  }
  return {};
}

// Whether T may block here. See executor.h.
bool ComponentExecutor::mayBlock(
    const Runtime::Component::Task &T) const noexcept {
  if (T.mayBlockAlways()) {
    return true;
  }
  // Any other ready thread of the instance tree makes blocking legal.
  const auto *Root = T.Opts.Inst != nullptr ? T.Opts.Inst->getRoot() : nullptr;
  const auto *Current = TaskMgr.getCurrentThread();
  for (const auto *Thread : TaskMgr.getWaitingThreads()) {
    if (Thread == Current || Thread->Finished || !Thread->ReadyFn ||
        !Thread->ReadyFn()) {
      continue;
    }
    const auto *Owner = Thread->getOwner();
    if (Owner == nullptr || Owner->Opts.Inst == nullptr ||
        Owner->Opts.Inst->getRoot() != Root) {
      continue;
    }
    return true;
  }
  return false;
}

// Whether Thread may take the stack under Pin's call. See executor.h.
bool ComponentExecutor::canResumeUnder(
    const Runtime::Component::Task &Pin,
    const Runtime::Component::TaskThread &Thread) const noexcept {
  const auto *Owner = Thread.getOwner();
  // A host task runs no guest code, so resuming it is never reentrance.
  if (Owner != nullptr && Owner->isHost()) {
    return true;
  }
  // Resuming a thread of another instance would be unexpected reentrance.
  if (Owner == nullptr || Owner->Opts.Inst != Pin.Opts.Inst) {
    return false;
  }
  // A task needing the stack exclusively cannot run under the pin.
  return Thread.ContextStack.empty() ||
         Thread.ContextStack.back() != &Owner->Implicit ||
         !Owner->needsExclusive();
}

} // namespace Executor
} // namespace WasmEdge
