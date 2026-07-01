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

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#define WASMEDGE_GC_DISABLE_SANITIZER
#else
#define WASMEDGE_GC_DISABLE_SANITIZER                                          \
  [[gnu::no_sanitize_address, gnu::no_sanitize_thread]]
#endif

namespace WasmEdge {

namespace Runtime {
namespace Instance {
class GlobalInstance;
class TableInstance;
class ElementInstance;
class ExceptionInstance;
} // namespace Instance
} // namespace Runtime

namespace GC {

class Allocator {
public:
  struct alignas(16) Header {
    uint32_t Size = 0;
    std::atomic<bool> IsGray = true;
  };

  Allocator();

  ~Allocator() noexcept;

  template <typename InitFunc>
  [[nodiscard]] void *allocate(InitFunc &&Init, uint32_t Size) noexcept {
    // Init runs inside this noexcept function; a throwing Init would terminate.
    static_assert(std::is_nothrow_invocable_v<InitFunc &, void *>,
                  "allocate()'s Init callback must be noexcept");
    // doAllocate and Header::Size are uint32_t; reject sizes where
    // sizeof(Header) + Size would wrap and under-allocate (Init then
    // overflows).
    if (unlikely(Size >
                 std::numeric_limits<uint32_t>::max() - sizeof(Header))) {
      return nullptr;
    }
    // Sum now fits uint32_t; narrow once (else MSVC /W4 flags the H->Size
    // assignment).
    const uint32_t Total = static_cast<uint32_t>(sizeof(Header) + Size);
    uint8_t *Pointer = doAllocate(Total);
    if (unlikely(!Pointer)) {
      return nullptr;
    }
    auto H = reinterpret_cast<Header *>(Pointer);
    new (H) Header();
    H->Size = Total;
    Init(reinterpret_cast<void *>(Pointer + sizeof(Header)));
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      Gray.push_back(H);
      // During concurrent marking, wake a worker to scan this freshly-allocated
      // gray object before sweeping; its edges would otherwise stay unmarked.
      // Guarded to MarkingGray so idle-time allocations don't wake workers.
      if (CurrentGCState.load(std::memory_order_acquire) ==
          GCState::MarkingGray) {
        GrayNotEmptyCV.notify_one();
      }
    }

    return static_cast<void *>(Pointer + sizeof(Header));
  }

  uint64_t getMemoryUsage() const noexcept {
    return Used.load(std::memory_order_relaxed);
  }

  void setManualGC(bool Enable) noexcept {
    EnableManualGC.store(Enable, std::memory_order_release);
  }

  // ScanNativeStack conservatively scans the current thread's native stack for
  // roots. Only AOT callers need it (a ref may live solely in a register/stack
  // slot); the interpreter keeps every live ref on a tracked value stack.
  void autoCollect(bool ScanNativeStack = false) noexcept;

  bool manualCollect(bool ScanNativeStack = false) noexcept;

  template <typename T> void writeBarrier(const T &Val) const noexcept {
    // Shade for any non-Idle state, not just MarkingGray: the collector can
    // enter MarkingRoot/MarkingGray between this load and the caller's store,
    // so guarding on MarkingGray alone would let a store during root marking
    // escape shading (lost object). markGray no-ops non-White refs, so shading
    // during MarkingRoot is merely conservative.
    if (likely(CurrentGCState.load(std::memory_order_acquire) ==
               GCState::Idle)) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
  }

  template <typename T>
  void bulkWriteBarrier(Span<const T> Slots) const noexcept {
    // See writeBarrier: shade for any non-Idle state to avoid losing stores
    // that race the MarkingRoot->MarkingGray transition.
    if (likely(CurrentGCState.load(std::memory_order_acquire) ==
               GCState::Idle)) {
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

  // Element segments holding GC refs are roots: a passive/declarative segment
  // can hold the only reference to a struct/array from its init expression,
  // which table.init/array.new_elem later copy into live tables/arrays.
  void addElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  void removeElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  // Exception payloads are roots: a struct/array ref in a tag payload may
  // survive only here once on-stack copies are consumed, and throw_ref
  // re-pushes the payload.
  void addException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  void
  removeException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  /// Host-root retention for GC references handed back to the host.
  ///
  /// retainResult pins a ref as a GC root. Refs match by pointer identity
  /// (ValType ignored) and retain by multiplicity: N retains need N releases.
  /// releaseRef removes one, releaseRefs one of each, releaseAllRefs all.
  void retainResult(const RefVariant &Ref) noexcept;

  WASMEDGE_EXPORT void releaseRef(const RefVariant &Ref) noexcept;

  void releaseRefs(Span<const RefVariant> Refs) noexcept {
    for (const auto &Ref : Refs) {
      releaseRef(Ref);
    }
  }

  WASMEDGE_EXPORT void releaseAllRefs() noexcept;

  /// Serialize a structural table-root mutation against the root scan, which
  /// reads every registered table's Refs vector under this same HeapMutex.
  /// growTable reallocates that vector; holding this lock across the resize
  /// stops the scan iterating a freed buffer. In-place slot writes
  /// (setRefs/fillRefs/setRefAddr) keep the buffer stable and don't need it --
  /// benign conservative-read races the scan tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockTableRoots() noexcept {
    return std::unique_lock<std::mutex>(HeapMutex);
  }

  /// Serialize a value-stack reallocation against the root scan, which iterates
  /// every registered ValueStack under this same StackMutex. A push past
  /// capacity reallocates and frees the buffer the scan is iterating; the
  /// pusher holds this lock only on that rare growth path (push/pushSpan check
  /// capacity first), so steady-state pushes stay lock-free. In-place writes
  /// (emplaceTop) and non-reallocating pushes keep the buffer stable -- benign
  /// conservative-read races the scan tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockStackRoots() noexcept {
    return std::unique_lock<std::mutex>(StackMutex);
  }

private:
  // Mark every registered root gray (value stacks, tables, globals, element
  // segments, exception payloads, host roots, and -- when ScanNativeStack --
  // the current thread's native stack). Shared by manual/autoCollect, which
  // differ only in how they enter the MarkingRoot phase.
  void markRoots(bool ScanNativeStack) noexcept;

  // Spill callee-saved registers onto the native stack, then mark every word of
  // the active stack as a conservative root. Must run in a frame that outlives
  // the synchronous root scan; see the definition for why the spill cannot live
  // inside getStack().
  void markNativeStackRoots() noexcept;

  // Active native stack span [Frame, stack base). Frame must point into a live
  // frame (typically a local's address in the caller) so the scan covers the
  // spilled registers and every caller frame up to the base.
  static Span<uint8_t *const> getStack(void *Frame) noexcept;

  [[nodiscard]] uint8_t *doAllocate(uint32_t N) noexcept;

  void doDeallocate(uint8_t *P, uint32_t Size) noexcept;

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const ValVariant &Val) noexcept {
    // The managed pointer is the high 64-bit word: RefVariant (and ValVariant's
    // RefVariant slot) is 128 bits, type tag in the low word, object pointer in
    // the high. Keep in sync with RefVariant in common/types.h -- a layout
    // change would silently make the GC miss every root. memcpy the bits out
    // rather than type-pun through an std::array reference (strict aliasing).
    std::array<uint64_t, 2> Raw;
    static_assert(sizeof(Val) >= sizeof(Raw));
    std::memcpy(Raw.data(), &Val, sizeof(Raw));
    return reinterpret_cast<uint8_t *>(Raw[1]);
  }

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const RefVariant &Ref) noexcept {
    std::array<uint64_t, 2> Raw;
    static_assert(sizeof(Ref) >= sizeof(Raw));
    std::memcpy(Raw.data(), &Ref, sizeof(Raw));
    return reinterpret_cast<uint8_t *>(Raw[1]);
  }

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(uint8_t *const &Ptr) noexcept { return Ptr; }

  void doWriteBarrier(uint8_t *Target) noexcept;

  // Lock whichever concrete mutex currently guards the named set (Black or
  // White), retrying if a concurrent Sweep() swaps the roles between loading
  // the pointer and taking the lock. On return the locked mutex still guards
  // the matching set, so the caller can mutate it without racing the swap.
  std::unique_lock<std::mutex>
  lockSetMutex(std::atomic<std::mutex *> &MutexPtr) noexcept;

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
  std::atomic<uint32_t> WorkerCount = 0;
  std::atomic<uint64_t> Threshold = uint64_t{1024} * 1024 * 1024; // 1 GiB
  std::atomic<uint64_t> Used = 0;

  std::mutex StackMutex{};
  std::vector<std::vector<ValVariant> *> Stacks;
  std::mutex HeapMutex{};
  std::vector<Runtime::Instance::TableInstance *> Heaps;
  std::mutex GlobalMutex{};
  std::vector<Runtime::Instance::GlobalInstance *> Globals;

  std::mutex ElemMutex{};
  std::vector<Runtime::Instance::ElementInstance *> Elems;

  std::mutex ExceptionMutex{};
  std::vector<Runtime::Instance::ExceptionInstance *> Exceptions;

  std::mutex HostRootsMutex{};
  std::vector<uint8_t *> HostRoots;

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
