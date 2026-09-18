// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/component/ComponentThreadTaskTest.cpp - Thread tests ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of the thread and task state machines of the
/// component runtime, driven the way the executor drives them.
///
//===----------------------------------------------------------------------===//

#include "common/errcode.h"
#include "runtime/component/task.h"
#include "runtime/component/thread.h"
#include "runtime/instance/component/component.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace {

using namespace WasmEdge;

// A detached host activation of Inst: async-typed, so it may always block.
std::unique_ptr<Runtime::Component::Task>
newHostTask(const Runtime::Instance::ComponentInstance &Inst) {
  return std::make_unique<Runtime::Component::Task>(&Inst, nullptr,
                                                    /*IsDetached=*/true);
}

// An implicit synchronous activation of Inst, as a start function runs under.
std::unique_ptr<Runtime::Component::Task>
newSyncTask(const Runtime::Instance::ComponentInstance &Inst) {
  return std::make_unique<Runtime::Component::Task>(&Inst, nullptr,
                                                    /*IsDetached=*/false);
}

bool isNever() { return false; }

// Thread: a stack of its own, parked until the executor hands it the stack.

TEST(ComponentThread, StartsSuspendedAndEndsWhenItsBodyReturns) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  bool Ran = false;
  Runtime::Component::Thread Cur(*T, [&Ran]() { Ran = true; });
  EXPECT_TRUE(Cur.isRoot());
  EXPECT_TRUE(Cur.isSuspended());
  EXPECT_FALSE(Cur.isRunning());
  EXPECT_FALSE(Cur.isWaiting());
  EXPECT_FALSE(Cur.isReady());
  EXPECT_FALSE(Cur.isEnded());
  EXPECT_EQ(Cur.getInnermost(), &Cur);
  EXPECT_EQ(&Cur.getOwner(), T.get());
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Ran);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, NeverResumedReleasesItsStackWithoutRunning) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  bool Ran = false;
  {
    Runtime::Component::Thread Cur(*T, [&Ran]() { Ran = true; });
    EXPECT_FALSE(Cur.isEnded());
  }
  EXPECT_FALSE(Ran);
}

TEST(ComponentThread, WaitParksUntilItsPredicateHolds) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  bool Flag = false;
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Cur(*T, [&]() {
    Woken = T->wait(Cur, [&Flag]() { return Flag; }, /*Cancellable=*/false);
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isWaiting());
  EXPECT_FALSE(Cur.isSuspended());
  EXPECT_FALSE(Cur.isReady());
  EXPECT_FALSE(Cur.isCancellable());
  EXPECT_FALSE(Cur.isEnded());
  Flag = true;
  EXPECT_TRUE(Cur.isReady());
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Normal);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, YieldParksReady) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  uint32_t Steps = 0;
  Runtime::Component::Thread Cur(*T, [&]() {
    Steps = 1;
    EXPECT_EQ(T->yield(Cur, /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
    Steps = 2;
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Steps, 1u);
  EXPECT_TRUE(Cur.isWaiting());
  EXPECT_TRUE(Cur.isReady());
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Steps, 2u);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, SuspendWaitsForANamedResume) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Cur(
      *T, [&]() { Woken = T->suspend(Cur, /*Cancellable=*/false); });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isSuspended());
  EXPECT_FALSE(Cur.isReady());
  Cur.onReady();
  EXPECT_TRUE(Cur.isWaiting());
  EXPECT_TRUE(Cur.isReady());
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Normal);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, CollectCompletesInPlaceWhenAlreadyReady) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Cur(*T, [&]() {
    Woken = T->collect(Cur, []() { return true; }, /*Cancellable=*/false);
  });
  // One hand-over runs the body to its end: nothing parked.
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Normal);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, CollectParksWhenNothingIsPending) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  bool Flag = false;
  Runtime::Component::Thread Cur(*T, [&]() {
    EXPECT_EQ(T->collect(
                  Cur, [&Flag]() { return Flag; }, /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isWaiting());
  EXPECT_FALSE(Cur.isEnded());
  Flag = true;
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, SwitchNamesTheNextThread) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  bool RanSecond = false;
  Runtime::Component::Thread::Reason WokenFirst =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Second(*T, [&RanSecond]() { RanSecond = true; });
  Runtime::Component::Thread First(*T, [&]() {
    WokenFirst =
        T->switchTo(First, Second, Runtime::Component::Thread::Next::Waiting,
                    /*Cancellable=*/false);
  });
  // First hands the stack over and names Second; it parks ready itself.
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            &Second);
  EXPECT_TRUE(First.isReady());
  EXPECT_FALSE(RanSecond);
  EXPECT_EQ(Second.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_TRUE(RanSecond);
  EXPECT_TRUE(Second.isEnded());
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_EQ(WokenFirst, Runtime::Component::Thread::Reason::Normal);
  EXPECT_TRUE(First.isEnded());
}

TEST(ComponentThread, SwitchMayLeaveTheSwitcherSuspended) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread Second(*T, []() {});
  Runtime::Component::Thread First(*T, [&]() {
    EXPECT_EQ(T->switchTo(First, Second,
                          Runtime::Component::Thread::Next::Suspended,
                          /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
  });
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            &Second);
  EXPECT_TRUE(First.isSuspended());
  EXPECT_EQ(Second.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  First.onReady();
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_TRUE(First.isEnded());
}

TEST(ComponentThread, PromoteHandsOverOnlyToAReadyThread) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread Second(*T, []() {});
  Runtime::Component::Thread First(*T, [&]() {
    EXPECT_EQ(T->promote(First, Second,
                         Runtime::Component::Thread::Next::Suspended,
                         /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
    EXPECT_EQ(T->promote(First, Second,
                         Runtime::Component::Thread::Next::Waiting,
                         /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
  });
  // Second is not ready: First parks as asked and names nobody.
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_TRUE(First.isSuspended());
  Second.onReady();
  First.onReady();
  // Second is ready now: First hands over to it.
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            &Second);
  EXPECT_TRUE(First.isReady());
  EXPECT_EQ(Second.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_EQ(First.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_TRUE(First.isEnded());
}

TEST(ComponentThread, ACancelledWakeReachesACancellableWait) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Cur(
      *T, [&]() { Woken = T->wait(Cur, isNever, /*Cancellable=*/true); });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isCancellable());
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Cancelled),
            nullptr);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Cancelled);
  EXPECT_FALSE(Cur.isCancellable());
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, AnAbortUnwindsAndRefusesLaterParks) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason First =
      Runtime::Component::Thread::Reason::Normal;
  Runtime::Component::Thread::Reason Second =
      Runtime::Component::Thread::Reason::Normal;
  bool Done = false;
  Runtime::Component::Thread Cur(*T, [&]() {
    First = T->wait(Cur, isNever, /*Cancellable=*/false);
    Second = T->yield(Cur, /*Cancellable=*/false);
    Done = true;
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  // The second park returns at once: the body runs to its end.
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Abort), nullptr);
  EXPECT_EQ(First, Runtime::Component::Thread::Reason::Abort);
  EXPECT_EQ(Second, Runtime::Component::Thread::Reason::Abort);
  EXPECT_TRUE(Done);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentThread, DestroyedWhileParkedUnwindsItsBody) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Normal;
  bool Done = false;
  {
    Runtime::Component::Thread Cur(*T, [&]() {
      Woken = T->wait(Cur, isNever, /*Cancellable=*/false);
      Done = true;
    });
    EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal),
              nullptr);
    EXPECT_TRUE(Cur.isWaiting());
  }
  EXPECT_TRUE(Done);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Abort);
}

TEST(ComponentThread, ANestedThreadSharesTheStackOfItsCaller) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto Caller = newHostTask(Inst);
  auto Callee = newSyncTask(Inst);
  Runtime::Component::Thread Outer(*Caller, [&]() {
    Runtime::Component::Thread Inner(*Callee, Outer);
    EXPECT_FALSE(Inner.isRoot());
    EXPECT_TRUE(Inner.isRunning());
    EXPECT_FALSE(Outer.isRunning());
    EXPECT_EQ(Outer.getInnermost(), &Inner);
    EXPECT_EQ(Inner.getInnermost(), &Inner);
    EXPECT_FALSE(Inner.isEnded());
    Inner.onEnd();
    EXPECT_TRUE(Inner.isEnded());
    EXPECT_EQ(Outer.getInnermost(), &Outer);
    EXPECT_TRUE(Outer.isRunning());
  });
  EXPECT_EQ(Outer.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_TRUE(Outer.isEnded());
}

TEST(ComponentThread, ANestedThreadParksTheWholeStack) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto Caller = newHostTask(Inst);
  auto Callee = newHostTask(Inst);
  bool Flag = false;
  uint32_t Steps = 0;
  Runtime::Component::Thread Outer(*Caller, [&]() {
    Runtime::Component::Thread Inner(*Callee, Outer);
    Steps = 1;
    EXPECT_EQ(Callee->wait(
                  Inner, [&Flag]() { return Flag; }, /*Cancellable=*/false),
              Runtime::Component::Thread::Reason::Normal);
    Steps = 2;
    Inner.onEnd();
  });
  EXPECT_EQ(Outer.onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_EQ(Steps, 1u);
  // The innermost thread is the one waiting; the outer one is not running.
  Runtime::Component::Thread *Inner = Outer.getInnermost();
  ASSERT_NE(Inner, &Outer);
  EXPECT_TRUE(Inner->isWaiting());
  EXPECT_FALSE(Inner->isReady());
  EXPECT_FALSE(Outer.isRunning());
  Flag = true;
  EXPECT_TRUE(Inner->isReady());
  EXPECT_EQ(Inner->onResume(Runtime::Component::Thread::Reason::Normal),
            nullptr);
  EXPECT_EQ(Steps, 2u);
  EXPECT_TRUE(Outer.isEnded());
}

TEST(ComponentThread, ASignalWakesAnAcquireFromAnotherThread) {
  Runtime::Component::Thread::Signal Signal;
  const auto Start = std::chrono::steady_clock::now();
  std::thread Releaser([&Signal]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Signal.release();
  });
  Signal.acquire();
  Releaser.join();
  EXPECT_GE(std::chrono::steady_clock::now() - Start,
            std::chrono::milliseconds(10));
}

TEST(ComponentThread, ASignalReleasedBeforeTheAcquireIsNotLost) {
  Runtime::Component::Thread::Signal Signal;
  Signal.release();
  const auto Start = std::chrono::steady_clock::now();
  EXPECT_TRUE(Signal.acquireUntil(Start + std::chrono::seconds(1)));
  EXPECT_LT(std::chrono::steady_clock::now() - Start, std::chrono::seconds(1));
}

TEST(ComponentThread, ASignalAcquireGivesUpAtTheDeadline) {
  Runtime::Component::Thread::Signal Signal;
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
  EXPECT_FALSE(Signal.acquireUntil(Deadline));
  EXPECT_GE(std::chrono::steady_clock::now(), Deadline);
}

TEST(ComponentThread, ContextStorageHasTwoSlots) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread Cur(*T, []() {});
  EXPECT_EQ(Cur.getStorage(0), 0u);
  EXPECT_EQ(Cur.getStorage(1), 0u);
  Cur.setStorage(0, 7);
  Cur.setStorage(1, 9);
  EXPECT_EQ(Cur.getStorage(0), 7u);
  EXPECT_EQ(Cur.getStorage(1), 9u);
}

// Task: the activation the threads run for.

TEST(ComponentTask, AnImplicitActivationStartsStartedAndReturnsOnce) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newSyncTask(Inst);
  EXPECT_FALSE(T->isFuncTypeAsync());
  EXPECT_FALSE(T->isHost());
  EXPECT_FALSE(T->isDetached());
  EXPECT_FALSE(T->isResolved());
  EXPECT_EQ(T->getInstance(), &Inst);
  EXPECT_EQ(T->getCaller(), nullptr);
  EXPECT_EQ(T->getSubtaskCode(), 1u);
  EXPECT_TRUE(T->onReturn({}));
  EXPECT_TRUE(T->isResolved());
  EXPECT_EQ(T->getSubtaskCode(), 2u);
  auto Again = T->onReturn({});
  ASSERT_FALSE(Again);
  EXPECT_EQ(Again.error(), ErrCode::Value::ComponentTaskResolvedTwice);
}

TEST(ComponentTask, AHostActivationMayAlwaysBlock) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  EXPECT_TRUE(T->isHost());
  EXPECT_TRUE(T->isDetached());
  EXPECT_TRUE(T->isFuncTypeAsync());
  EXPECT_FALSE(T->needsExclusive());
  EXPECT_TRUE(T->mayBlockAlways());
  Runtime::Component::Thread Cur(*T, []() {});
  EXPECT_TRUE(T->mayBlock(Cur, {}));
}

TEST(ComponentTask, CancelNeedsADeliveredRequest) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newSyncTask(Inst);
  auto Early = T->onCancel();
  ASSERT_FALSE(Early);
  EXPECT_EQ(Early.error(), ErrCode::Value::ComponentTaskNotCancelled);
  // No thread is parked cancellably: the request queues, and the subtask
  // view records it so that a second `subtask.cancel` traps.
  EXPECT_FALSE(T->isCancelRequested());
  EXPECT_EQ(T->onCancelRequest(), nullptr);
  EXPECT_TRUE(T->isCancelRequested());
  EXPECT_FALSE(T->onCancelDelivery(/*Cancellable=*/false));
  EXPECT_TRUE(T->onCancelDelivery(/*Cancellable=*/true));
  EXPECT_FALSE(T->onCancelDelivery(/*Cancellable=*/true));
  EXPECT_TRUE(T->onCancel());
  EXPECT_TRUE(T->isResolved());
  EXPECT_EQ(T->getSubtaskCode(), 4u);
}

TEST(ComponentTask, ACancelRequestNamesTheThreadParkedCancellably) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason Woken =
      Runtime::Component::Thread::Reason::Normal;
  Runtime::Component::Thread Cur(*T, [&]() {
    EXPECT_EQ(T->onEnter(Cur), Runtime::Component::Thread::Reason::Normal);
    EXPECT_EQ(T->getImplicitThread(), &Cur);
    Woken = T->wait(Cur, isNever, /*Cancellable=*/true);
    EXPECT_TRUE(T->onCancel());
    EXPECT_TRUE(T->onExit());
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  // Entering registered the thread in the instance table.
  EXPECT_NE(Cur.getIndex(), 0u);
  ASSERT_EQ(T->getThreads().size(), 1u);
  EXPECT_EQ(T->getThreads()[0], &Cur);
  EXPECT_EQ(T->onCancelRequest(), &Cur);
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Cancelled),
            nullptr);
  EXPECT_EQ(Woken, Runtime::Component::Thread::Reason::Cancelled);
  EXPECT_TRUE(T->isResolved());
  EXPECT_EQ(T->getSubtaskCode(), 4u);
  EXPECT_EQ(Cur.getIndex(), 0u);
  EXPECT_TRUE(T->getThreads().empty());
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentTask, AQueuedCancellationIsDeliveredAtTheNextCancellablePark) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread::Reason First =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread::Reason Second =
      Runtime::Component::Thread::Reason::Abort;
  Runtime::Component::Thread Cur(*T, [&]() {
    EXPECT_EQ(T->onEnter(Cur), Runtime::Component::Thread::Reason::Normal);
    First = T->wait(Cur, isNever, /*Cancellable=*/false);
    // The queued cancellation returns at once, without parking.
    Second = T->yield(Cur, /*Cancellable=*/true);
    EXPECT_TRUE(T->onCancel());
    EXPECT_TRUE(T->onExit());
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_FALSE(Cur.isCancellable());
  EXPECT_EQ(T->onCancelRequest(), nullptr);
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(First, Runtime::Component::Thread::Reason::Normal);
  EXPECT_EQ(Second, Runtime::Component::Thread::Reason::Cancelled);
  EXPECT_TRUE(T->isResolved());
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentTask, TheLastThreadToLeaveOwesAResult) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread Cur(*T, [&]() {
    EXPECT_EQ(T->onEnter(Cur), Runtime::Component::Thread::Reason::Normal);
    auto Exit = T->onExit();
    ASSERT_FALSE(Exit);
    EXPECT_EQ(Exit.error(), ErrCode::Value::ComponentNoAsyncResult);
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_TRUE(Cur.isEnded());
}

TEST(ComponentTask, ExitAfterTheResultLeavesNoThreadBehind) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newHostTask(Inst);
  Runtime::Component::Thread Cur(*T, [&]() {
    EXPECT_EQ(T->onEnter(Cur), Runtime::Component::Thread::Reason::Normal);
    EXPECT_TRUE(T->onReturn({}));
    EXPECT_TRUE(T->onExit());
  });
  EXPECT_EQ(Cur.onResume(Runtime::Component::Thread::Reason::Normal), nullptr);
  EXPECT_EQ(Cur.getIndex(), 0u);
  EXPECT_TRUE(T->getThreads().empty());
  EXPECT_TRUE(T->isResolved());
}

TEST(ComponentTask, BorrowsMustBeDroppedBeforeReturning) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newSyncTask(Inst);
  T->addBorrow();
  auto Held = T->onReturn({});
  ASSERT_FALSE(Held);
  EXPECT_EQ(Held.error(), ErrCode::Value::ComponentBorrowsRemain);
  T->dropBorrow();
  EXPECT_TRUE(T->onReturn({}));
}

TEST(ComponentTask, LendersAreReleasedWithTheResolution) {
  Runtime::Instance::ComponentInstance Inst("a");
  auto T = newSyncTask(Inst);
  Runtime::Instance::ComponentInstance::ResourceHandle Handle;
  T->addLender(&Handle);
  T->addLender(&Handle);
  EXPECT_EQ(Handle.NumLends, 2u);
  T->releaseLenders();
  EXPECT_EQ(Handle.NumLends, 0u);
  T->releaseLenders();
  EXPECT_EQ(Handle.NumLends, 0u);
}

TEST(ComponentTask,
     ASynchronousActivationBlocksOnlyWhenItsTreeHasAReadyThread) {
  Runtime::Instance::ComponentInstance InstA("a");
  Runtime::Instance::ComponentInstance InstB("b");
  auto Sync = newSyncTask(InstA);
  auto Sibling = newHostTask(InstA);
  auto Foreign = newHostTask(InstB);
  Runtime::Component::Thread Cur(*Sync, []() {});
  Runtime::Component::Thread SiblingThread(*Sibling, []() {});
  Runtime::Component::Thread ForeignThread(*Foreign, []() {});
  std::vector<Runtime::Component::Thread *> Waiting{&Cur, &SiblingThread,
                                                    &ForeignThread};
  EXPECT_FALSE(Sync->mayBlockAlways());
  EXPECT_FALSE(Sync->mayBlock(Cur, Waiting));
  // A ready thread of another instance tree does not count.
  ForeignThread.onReady();
  EXPECT_FALSE(Sync->mayBlock(Cur, Waiting));
  SiblingThread.onReady();
  EXPECT_TRUE(Sync->mayBlock(Cur, Waiting));
  // Neither does the blocking thread itself.
  Cur.onReady();
  std::vector<Runtime::Component::Thread *> Alone{&Cur};
  EXPECT_FALSE(Sync->mayBlock(Cur, Alone));
  // Once resolved, the activation may always block.
  EXPECT_TRUE(Sync->onReturn({}));
  EXPECT_TRUE(Sync->mayBlockAlways());
  EXPECT_TRUE(Sync->mayBlock(Cur, Alone));
}

TEST(ComponentTask, ResumingUnderASynchronousCallStaysWithinTheInstance) {
  Runtime::Instance::ComponentInstance InstA("a");
  Runtime::Instance::ComponentInstance InstB("b");
  auto Call = newSyncTask(InstA);
  auto HostOfA = newHostTask(InstA);
  auto HostOfB = newHostTask(InstB);
  auto SyncOfA = newSyncTask(InstA);
  auto SyncOfB = newSyncTask(InstB);
  Runtime::Component::Thread HostA(*HostOfA, []() {});
  Runtime::Component::Thread HostB(*HostOfB, []() {});
  Runtime::Component::Thread GuestA(*SyncOfA, []() {});
  Runtime::Component::Thread GuestB(*SyncOfB, []() {});
  // Host threads run no guest code; guest threads only of the same instance.
  EXPECT_TRUE(Call->mayResume(HostA));
  EXPECT_TRUE(Call->mayResume(HostB));
  EXPECT_TRUE(Call->mayResume(GuestA));
  EXPECT_FALSE(Call->mayResume(GuestB));
}

TEST(ComponentTask, IsUnderRootFollowsTheCallers) {
  Runtime::Instance::ComponentInstance InstA("a");
  Runtime::Instance::ComponentInstance InstB("b");
  auto Outer = newSyncTask(InstA);
  Runtime::Component::Task Inner(&InstB, Outer.get(), /*IsDetached=*/false);
  EXPECT_EQ(Inner.getCaller(), Outer.get());
  EXPECT_TRUE(Inner.isUnderRoot(&InstB));
  EXPECT_TRUE(Inner.isUnderRoot(&InstA));
  EXPECT_FALSE(Outer->isUnderRoot(&InstB));
}

} // namespace
