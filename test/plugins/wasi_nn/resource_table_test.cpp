// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "resource_table.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <vector>

namespace {

using WasmEdge::Host::WASINN::ResourceTable;

// Payload that counts its destructions, so the tests can pin exactly where the
// final release happens relative to table operations.
struct Tracked {
  explicit Tracked(std::atomic<uint32_t> &Counter) noexcept
      : Destroyed(Counter) {}
  ~Tracked() noexcept { Destroyed.fetch_add(1, std::memory_order_relaxed); }
  std::atomic<uint32_t> &Destroyed;
};

TEST(WasiNNResourceTableTest, InsertAllocatesSequentialIds) {
  ResourceTable<int> Table;
  for (uint32_t Expected = 0; Expected < 8; ++Expected) {
    const auto Id = Table.insert(std::make_shared<int>(0));
    ASSERT_TRUE(Id.has_value());
    EXPECT_EQ(*Id, Expected);
  }
  EXPECT_EQ(Table.size(), 8U);
}

TEST(WasiNNResourceTableTest, GetResolvesLiveHandleAndRejectsUnknown) {
  ResourceTable<int> Table;
  const auto Id = Table.insert(std::make_shared<int>(42));
  ASSERT_TRUE(Id.has_value());
  const auto Value = Table.get(*Id);
  ASSERT_NE(Value, nullptr);
  EXPECT_EQ(*Value, 42);
  EXPECT_EQ(Table.get(*Id + 1), nullptr);
  EXPECT_EQ(Table.get(std::numeric_limits<uint32_t>::max()), nullptr);
}

TEST(WasiNNResourceTableTest, RemoveDetachesHandleAndReturnsOwnership) {
  std::atomic<uint32_t> Destroyed{0};
  ResourceTable<Tracked> Table;
  const auto Id = Table.insert(std::make_shared<Tracked>(Destroyed));
  ASSERT_TRUE(Id.has_value());

  auto Detached = Table.remove(*Id);
  ASSERT_NE(Detached, nullptr);
  EXPECT_EQ(Table.get(*Id), nullptr);
  EXPECT_EQ(Table.size(), 0U);
  EXPECT_EQ(Destroyed.load(), 0U);

  Detached.reset();
  EXPECT_EQ(Destroyed.load(), 1U);
}

TEST(WasiNNResourceTableTest, DoubleRemoveIsRejected) {
  ResourceTable<int> Table;
  const auto Id = Table.insert(std::make_shared<int>(0));
  ASSERT_TRUE(Id.has_value());
  EXPECT_NE(Table.remove(*Id), nullptr);
  EXPECT_EQ(Table.remove(*Id), nullptr);
}

TEST(WasiNNResourceTableTest, IdsAreNeverReused) {
  ResourceTable<int> Table;
  const auto First = Table.insert(std::make_shared<int>(1));
  ASSERT_TRUE(First.has_value());
  ASSERT_NE(Table.remove(*First), nullptr);

  const auto Second = Table.insert(std::make_shared<int>(2));
  ASSERT_TRUE(Second.has_value());
  EXPECT_NE(*Second, *First);

  // The stale handle keeps missing instead of aliasing the new resource.
  EXPECT_EQ(Table.get(*First), nullptr);
  const auto Fresh = Table.get(*Second);
  ASSERT_NE(Fresh, nullptr);
  EXPECT_EQ(*Fresh, 2);
}

TEST(WasiNNResourceTableTest, PinnedResourceSurvivesRemoval) {
  std::atomic<uint32_t> Destroyed{0};
  ResourceTable<Tracked> Table;
  const auto Id = Table.insert(std::make_shared<Tracked>(Destroyed));
  ASSERT_TRUE(Id.has_value());

  // An in-flight operation holds its pin across a concurrent unload.
  auto Pin = Table.get(*Id);
  ASSERT_NE(Pin, nullptr);
  Table.remove(*Id).reset();
  EXPECT_EQ(Destroyed.load(), 0U);

  // The final release runs the destructor, wherever it happens.
  Pin.reset();
  EXPECT_EQ(Destroyed.load(), 1U);
}

// The table mutex is never held while a payload destructor runs, so a
// destructor may call back into the same table without deadlocking. This pins
// the "destruction off the table lock" contract with a non-recursive mutex.
struct Reentrant {
  explicit Reentrant(ResourceTable<Reentrant> &Owner) noexcept : Owner(Owner) {}
  ~Reentrant() noexcept { ObservedSize = Owner.size(); }
  ResourceTable<Reentrant> &Owner;
  static uint32_t ObservedSize;
};
uint32_t Reentrant::ObservedSize = std::numeric_limits<uint32_t>::max();

TEST(WasiNNResourceTableTest, DestructorMayReenterTable) {
  ResourceTable<Reentrant> Table;
  const auto Id = Table.insert(std::make_shared<Reentrant>(Table));
  ASSERT_TRUE(Id.has_value());
  Table.remove(*Id).reset();
  EXPECT_EQ(Reentrant::ObservedSize, 0U);
}

TEST(WasiNNResourceTableTest, InsertFailsOnIdExhaustion) {
  ResourceTable<int> Table(std::numeric_limits<uint32_t>::max() - 1);
  const auto Last = Table.insert(std::make_shared<int>(0));
  ASSERT_TRUE(Last.has_value());
  EXPECT_EQ(*Last, std::numeric_limits<uint32_t>::max() - 1);
  EXPECT_EQ(Table.insert(std::make_shared<int>(0)), std::nullopt);
  // The failed insert did not disturb the surviving entry.
  EXPECT_EQ(Table.size(), 1U);
  EXPECT_NE(Table.get(*Last), nullptr);
}

// Concurrency stress: producers publish, workers pin-and-use, removers detach.
// Run under TSan this exercises the whole contract: unique ids, no lost or
// double destruction, pins keeping payloads alive across concurrent removal.
TEST(WasiNNResourceTableTest, ConcurrentChurnKeepsOwnershipExact) {
  constexpr uint32_t Producers = 4;
  constexpr uint32_t PerProducer = 256;
  constexpr uint32_t Empty = std::numeric_limits<uint32_t>::max();
  std::atomic<uint32_t> Destroyed{0};
  ResourceTable<Tracked> Table;

  // Each producer owns a disjoint slice; a slot flips from Empty to its id
  // with release ordering, so readers only ever act on fully published ids.
  struct Slot {
    std::atomic<uint32_t> Id{Empty};
  };
  std::vector<Slot> Published(Producers * PerProducer);
  std::atomic<bool> Done{false};

  std::vector<std::thread> Threads;
  for (uint32_t P = 0; P < Producers; ++P) {
    Threads.emplace_back([&Table, &Destroyed, &Published, P] {
      for (uint32_t I = 0; I < PerProducer; ++I) {
        const auto Id = Table.insert(std::make_shared<Tracked>(Destroyed));
        ASSERT_TRUE(Id.has_value());
        Published[P * PerProducer + I].Id.store(*Id, std::memory_order_release);
      }
    });
  }
  // Removers race the producers over every published slot.
  for (uint32_t R = 0; R < 2; ++R) {
    Threads.emplace_back([&Table, &Published, &Done] {
      while (!Done.load(std::memory_order_acquire)) {
        for (auto &Slot : Published) {
          const uint32_t Id = Slot.Id.load(std::memory_order_acquire);
          if (Id != Empty) {
            Table.remove(Id);
          }
        }
      }
    });
  }
  // Readers pin whatever they can catch; the pin must stay usable even when a
  // remover detaches the entry mid-use.
  for (uint32_t W = 0; W < 2; ++W) {
    Threads.emplace_back([&Table, &Published, &Destroyed, &Done] {
      while (!Done.load(std::memory_order_acquire)) {
        for (auto &Slot : Published) {
          const uint32_t Id = Slot.Id.load(std::memory_order_acquire);
          if (Id == Empty) {
            continue;
          }
          if (auto Pin = Table.get(Id)) {
            // Touch the payload through the pin: alive even if removed now.
            EXPECT_EQ(&Pin->Destroyed, &Destroyed);
          }
        }
      }
    });
  }

  for (uint32_t P = 0; P < Producers; ++P) {
    Threads[P].join();
  }
  Done.store(true, std::memory_order_release);
  for (uint32_t I = Producers; I < Threads.size(); ++I) {
    Threads[I].join();
  }

  // Drain anything the removers did not catch, then check exact ownership.
  std::set<uint32_t> Unique;
  for (auto &Slot : Published) {
    const uint32_t Id = Slot.Id.load(std::memory_order_acquire);
    ASSERT_NE(Id, Empty);
    Unique.insert(Id);
    Table.remove(Id);
  }
  EXPECT_EQ(Table.size(), 0U);
  EXPECT_EQ(Destroyed.load(), Producers * PerProducer);
  EXPECT_EQ(Unique.size(), Published.size());
}

} // namespace
