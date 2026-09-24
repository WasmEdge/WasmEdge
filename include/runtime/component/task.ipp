// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/task.ipp - Task definition -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the members of the task that work on its component
/// instance; "runtime/instance/component/component.h" includes it.
///
//===----------------------------------------------------------------------===//

namespace WasmEdge {
namespace Runtime {
namespace Component {

inline Thread::WakeReason Task::onEnter(Thread &Cur) noexcept {
  ImplicitThread = &Cur;
  auto *Inst = Opts.Inst;
  if (FuncTypeAsync && !isHost()) {
    auto HasBackpressure = [this, Inst]() {
      return Inst->getBackpressure() > 0 ||
             (needsExclusive() && Inst->getExclusiveThread() != nullptr);
    };
    if (HasBackpressure() || Inst->getNumWaitingToEnter() > 0) {
      Inst->incWaitingToEnter();
      // A cancellation resumes the gated thread whether or not it is ready.
      const Thread::WakeReason Wake =
          wait(Cur, [HasBackpressure]() { return !HasBackpressure(); });
      // A teardown wake-up must not touch possibly-gone instance state.
      if (Wake == Thread::WakeReason::Aborted) {
        return Wake;
      }
      Inst->decWaitingToEnter();
      if (onCancelDelivery()) {
        return Wake;
      }
    }
    if (needsExclusive()) {
      takeExclusive();
    }
  }
  addThread(Cur);
  return Thread::WakeReason::Normal;
}

inline void Task::onAbort() noexcept {
  // A thread aborted at the entry gate still counts as waiting to enter.
  if (Opts.Inst != nullptr && ImplicitThread != nullptr && Threads.empty() &&
      SubtaskStatus == SubtaskState::Starting && FuncTypeAsync && !isHost()) {
    Opts.Inst->decWaitingToEnter();
  }
  if (ImplicitThread != nullptr && hasExclusive()) {
    releaseExclusive();
  }
  releaseLenders();
  if (Status != TaskState::Resolved) {
    Status = TaskState::Aborted;
  }
  ImplicitThread = nullptr;
  Threads.clear();
  Opts.Inst = nullptr;
  releaseCaller();
}

inline Thread *Task::onCancelRequest() noexcept {
  CancelRequested = true;
  if (Status != TaskState::Initial) {
    return nullptr;
  }
  Status = TaskState::PendingCancel;
  // Not started yet: the gated thread delivers it at the gate.
  if (SubtaskStatus == SubtaskState::Starting) {
    if (ImplicitThread != nullptr && !ImplicitThread->isRunning()) {
      return ImplicitThread;
    }
    return nullptr;
  }
  // Started: any ready thread of the instance may take the turn; one of the
  // task itself goes first.
  for (Thread *Entry : Threads) {
    if (Entry->isReady()) {
      return Entry;
    }
  }
  if (Opts.Inst != nullptr) {
    for (Thread *Entry : Opts.Inst->getThreads()) {
      if (Entry != nullptr && Entry->isReady()) {
        return Entry;
      }
    }
  }
  return nullptr;
}

inline void Task::addThread(Thread &Entry) noexcept {
  Threads.push_back(&Entry);
  if (Opts.Inst != nullptr) {
    Entry.setIndex(Opts.Inst->addThread(&Entry));
  }
}

inline Expect<void> Task::removeThread(Thread &Entry) noexcept {
  Threads.erase(std::remove(Threads.begin(), Threads.end(), &Entry),
                Threads.end());
  if (Entry.getIndex() != 0 && Opts.Inst != nullptr) {
    Opts.Inst->removeThread(Entry.getIndex());
  }
  Entry.setIndex(0);
  if (Threads.empty() && Status != TaskState::Resolved) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  return {};
}

inline void
Task::addLender(Instance::Component::ResourceHandle *Handle) noexcept {
  Handle->NumLends += 1;
  Lenders.push_back(Handle);
}

inline void Task::releaseLenders() noexcept {
  for (auto *Handle : Lenders) {
    if (Handle->NumLends > 0) {
      Handle->NumLends -= 1;
    }
  }
  Lenders.clear();
}

inline bool Task::isExclusiveFree() const noexcept {
  return Opts.Inst == nullptr || Opts.Inst->getExclusiveThread() == nullptr;
}

inline bool Task::hasExclusive() const noexcept {
  return Opts.Inst != nullptr && ImplicitThread != nullptr &&
         Opts.Inst->getExclusiveThread() == ImplicitThread;
}

inline void Task::takeExclusive() noexcept {
  if (Opts.Inst != nullptr) {
    Opts.Inst->setExclusiveThread(ImplicitThread);
  }
}

inline void Task::releaseExclusive() noexcept {
  if (Opts.Inst != nullptr) {
    Opts.Inst->setExclusiveThread(nullptr);
  }
}
} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
