// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/test/reflifetime/RefLifetimeTest.cpp ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for RefLifetime: the packed owner-flag + dependent-count state machine
/// and the concurrent owner/last-dependent release that must delete exactly
/// once.
///
//===----------------------------------------------------------------------===//

#include "runtime/instance/reflifetime.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace {

using WasmEdge::Runtime::Instance::RefLifetime;

TEST(RefLifetimeTest, OwnerOnlyDeletesOnReleaseOwner) {
  RefLifetime Life;
  EXPECT_FALSE(Life.hasDependents());
  EXPECT_TRUE(Life.releaseOwner());
}

TEST(RefLifetimeTest, ReleaseOwnerIsIdempotent) {
  RefLifetime Life;
  EXPECT_TRUE(Life.releaseOwner());
  EXPECT_FALSE(Life.releaseOwner());
}

TEST(RefLifetimeTest, OwnerWithDependentDefersUntilLastRelease) {
  RefLifetime Life;
  Life.addDependent();
  EXPECT_TRUE(Life.hasDependents());
  EXPECT_FALSE(Life.releaseOwner());
  EXPECT_TRUE(Life.releaseDependent());
}

TEST(RefLifetimeTest, DependentReleasedBeforeOwnerDoesNotDelete) {
  RefLifetime Life;
  Life.addDependent();
  EXPECT_FALSE(Life.releaseDependent());
  EXPECT_FALSE(Life.hasDependents());
  EXPECT_TRUE(Life.releaseOwner());
}

TEST(RefLifetimeTest, MultipleDependentsDeleteOnLast) {
  RefLifetime Life;
  Life.addDependent();
  Life.addDependent();
  Life.addDependent();
  EXPECT_FALSE(Life.releaseOwner());
  EXPECT_FALSE(Life.releaseDependent());
  EXPECT_FALSE(Life.releaseDependent());
  EXPECT_TRUE(Life.releaseDependent());
}

TEST(RefLifetimeTest, ConcurrentReleasesDeleteExactlyOnce) {
  constexpr int Dependents = 8;
  constexpr int Iterations = 256;
  for (int Iter = 0; Iter < Iterations; ++Iter) {
    // Real heap object guarded by Life, so ASan/TSan catches a premature or
    // double delete.
    auto *Payload = new std::atomic<int>(0);
    RefLifetime Life;
    for (int I = 0; I < Dependents; ++I) {
      Life.addDependent();
    }
    std::atomic<int> Deletes{0};
    std::atomic<bool> Start{false};
    std::vector<std::thread> Threads;
    Threads.reserve(Dependents + 1);
    for (int I = 0; I < Dependents; ++I) {
      Threads.emplace_back([&Life, &Deletes, &Start, Payload] {
        while (!Start.load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
        // The pin this thread still holds keeps Payload alive for this access.
        Payload->fetch_add(0, std::memory_order_relaxed);
        if (Life.releaseDependent()) {
          delete Payload;
          Deletes.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }
    Threads.emplace_back([&Life, &Deletes, &Start, Payload] {
      while (!Start.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      Payload->fetch_add(0, std::memory_order_relaxed);
      if (Life.releaseOwner()) {
        delete Payload;
        Deletes.fetch_add(1, std::memory_order_relaxed);
      }
    });
    Start.store(true, std::memory_order_release);
    for (auto &T : Threads) {
      T.join();
    }
    EXPECT_EQ(Deletes.load(), 1) << "iteration " << Iter;
  }
}

} // namespace
