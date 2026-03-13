// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/gc/allocator.h - GC memory allocator ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of allocator class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "system/allocator.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_set>

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#define WASMEDGE_GC_DISABLE_SANITIZER
#else
#define WASMEDGE_GC_DISABLE_SANITIZER                                          \
  [[gnu::no_sanitize_address, gnu::no_sanitize_thread]]
#endif

namespace WasmEdge {

namespace Runtime::Instance {
class GlobalInstance;
class TableInstance;
} // namespace Runtime::Instance

namespace GC {

class Allocator {
public:
  struct alignas(16) Header {
    uint32_t Size = 0;
    std::atomic<bool> IsGray = true;
  };
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(push)
#pragma warning(disable : 4200)
#endif
  struct Work {
    Work *Next = nullptr;
    uint32_t Size = 0;
    Header *Data[];
  };
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(pop)
#endif

  Allocator();

  ~Allocator() noexcept;

  template <typename InitFunc>
  [[nodiscard]] void *allocate(InitFunc &&Init, uint32_t Size) noexcept {
    uint8_t *Pointer = do_allocate(sizeof(Header) + Size);
    if (unlikely(!Pointer)) {
      return nullptr;
    }
    auto H = reinterpret_cast<Header *>(Pointer);
    new (H) Header();
    H->Size = sizeof(Header) + Size;
    Init(reinterpret_cast<void *>(Pointer + sizeof(Header)));
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      Gray.push_back(H);
    }

    return static_cast<void *>(Pointer + sizeof(Header));
  }

  uint64_t getMemoryUsage() const noexcept {
    return Used.load(std::memory_order_relaxed);
  }

  void setManualGC(bool Enable) noexcept {
    EnableManualGC.store(Enable, std::memory_order_release);
  }

  void autoCollect(Span<uint8_t *const> Stack = {}) noexcept;

  bool manualCollect(Span<uint8_t *const> Stack = {}) noexcept;

  template <typename T> void writeBarrier(const T &Val) const noexcept {
    if (likely(CurrentGCState.load(std::memory_order_acquire) !=
               GCState::MarkingGray)) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
  }

  template <typename T>
  void bulkWriteBarrier(Span<const T> Slots) const noexcept {
    if (likely(CurrentGCState.load(std::memory_order_acquire) !=
               GCState::MarkingGray)) {
      return;
    }
    for (const auto &Val : Slots) {
      const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
    }
  }

  void addStack(std::vector<ValVariant> &Vector) noexcept;

  void removeStack(std::vector<ValVariant> &Vector) noexcept;

  void addTable(Runtime::Instance::TableInstance &Table) noexcept;

  void removeTable(Runtime::Instance::TableInstance &Table) noexcept;

  void addGlobal(Runtime::Instance::GlobalInstance &Global) noexcept;

  void removeGlobal(Runtime::Instance::GlobalInstance &Global) noexcept;

  static Span<uint8_t *const> getStack() noexcept;

private:
  [[nodiscard]] uint8_t *do_allocate(uint32_t N) noexcept;

  void do_deallocate(uint8_t *P, uint32_t Size) noexcept;

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const ValVariant &Val) noexcept {
    return reinterpret_cast<uint8_t *>(
        reinterpret_cast<const std::array<uint64_t, 2> &>(Val)[1]);
  }

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const RefVariant &Ref) noexcept {
    return reinterpret_cast<uint8_t *>(
        reinterpret_cast<const std::array<uint64_t, 2> &>(Ref)[1]);
  }

  static uint8_t *getPointer(uint8_t *const &Ptr) noexcept { return Ptr; }

  void doWriteBarrier(uint8_t *Target) noexcept;

  void markGray(uint8_t *Pointer) noexcept;

  enum class GCState : uint8_t {
    Idle,
    MarkingRoot,
    MarkingGray,
    Sweeping,
  };

  std::atomic<bool> Stop = false;
  std::atomic<bool> EnableManualGC = false;
  std::atomic<GCState> CurrentGCState = GCState::Idle;
  std::atomic<uint32_t> WorkerCount;
  std::atomic<uint64_t> Threshold = 1024 * 1024;
  std::atomic<uint64_t> Used = 0;

  std::mutex StackMutex{};
  std::vector<std::vector<ValVariant> *> Stacks;
  std::mutex HeapMutex{};
  std::vector<Runtime::Instance::TableInstance *> Heaps;
  std::mutex GlobalMutex{};
  std::vector<Runtime::Instance::GlobalInstance *> Globals;

  std::mutex Set1Mutex{};
  std::unordered_set<Header *> Set1;

  std::mutex Set2Mutex{};
  std::unordered_set<Header *> Set2;

  std::mutex GrayMutex{};
  std::condition_variable GrayNotEmptyCV;
  std::deque<Header *> Gray;

  std::mutex GCMutex{};
  std::condition_variable GCCV;

  std::atomic<std::mutex *> BlackMutex = &Set1Mutex;
  std::atomic<std::unordered_set<Header *> *> Black = &Set1;

  std::atomic<std::mutex *> WhiteMutex = &Set2Mutex;
  std::atomic<std::unordered_set<Header *> *> White = &Set2;

  std::atomic<std::chrono::steady_clock::time_point> NextGC =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);

  std::vector<std::thread> Collectors;
};

} // namespace GC
} // namespace WasmEdge
