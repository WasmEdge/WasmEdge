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

namespace Runtime::Instance {
class GlobalInstance;
class TableInstance;
class ElementInstance;
class ExceptionInstance;
} // namespace Runtime::Instance

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
    // allocate() is noexcept and runs Init inside the block; a throwing Init
    // would terminate, so require it to be nothrow-invocable.
    static_assert(std::is_nothrow_invocable_v<InitFunc &, void *>,
                  "allocate()'s Init callback must be noexcept");
    // Reject sizes that would overflow the uint32_t block size: sizeof(Header)
    // + Size is computed in size_t but doAllocate and Header::Size are
    // uint32_t, so an unchecked sum wraps, under-allocates, and Init then
    // writes past the block (heap overflow).
    if (unlikely(Size >
                 std::numeric_limits<uint32_t>::max() - sizeof(Header))) {
      return nullptr;
    }
    // The overflow guard above bounds sizeof(Header) + Size to uint32_t, so the
    // narrowing is well-defined; compute it once (avoids an implicit
    // size_t->uint32_t narrowing on the H->Size assignment that MSVC /W4
    // flags).
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
      // If a collection is in its concurrent marking phase, wake a worker so
      // this freshly-allocated (gray) object is scanned in the current cycle;
      // otherwise its outgoing edges could remain unmarked before sweeping.
      // Guarded to MarkingGray so allocations outside a collection do not spin
      // workers up while the collector is idle.
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

  // ScanNativeStack additionally scans the current thread's native stack
  // conservatively for GC roots. Only AOT-compiled callers (which may hold a
  // reference solely in a machine register / native stack slot) need it; the
  // interpreter keeps every live reference on a tracked value stack.
  void autoCollect(bool ScanNativeStack = false) noexcept;

  bool manualCollect(bool ScanNativeStack = false) noexcept;

  template <typename T> void writeBarrier(const T &Val) const noexcept {
    // Run the barrier for any non-Idle state, not just MarkingGray: the
    // collector can transition into MarkingRoot/MarkingGray between this load
    // and the caller's subsequent reference store, so guarding only on
    // MarkingGray lets a store during root marking escape shading (lost
    // object). markGray no-ops references not in the White set, so shading
    // during MarkingRoot is safe and merely conservative.
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

  // Element segments holding GC references are roots: a passive/declarative
  // segment can hold the only reference to a struct/array produced by a
  // struct.new/array.new in its init expression, and table.init/array.new_elem
  // later copy those references into live tables/arrays.
  void addElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  void removeElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  // Exception instances retain their payload values; a struct/array reference
  // in a tag payload survives only here once the on-stack copies are consumed,
  // and throw_ref re-pushes the payload, so the payloads must be scanned.
  void addException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  void
  removeException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  /// Host-root retention for GC references handed back to the host.
  ///
  /// retainResult pins a reference as a GC root so the collector keeps the
  /// pointed-to object alive. References are matched by pointer identity
  /// (ValType is ignored), and retention is by multiplicity: a reference
  /// retained N times must be released N times. releaseRef removes one
  /// retained instance, releaseRefs removes one instance of each, and
  /// releaseAllRefs drops every retained reference.
  void retainResult(const RefVariant &Ref) noexcept;

  WASMEDGE_EXPORT void releaseRef(const RefVariant &Ref) noexcept;

  void releaseRefs(Span<const RefVariant> Refs) noexcept {
    for (const auto &Ref : Refs) {
      releaseRef(Ref);
    }
  }

  WASMEDGE_EXPORT void releaseAllRefs() noexcept;

  /// Serialize a structural table-root mutation against the collector's root
  /// scan. TableInstance::growTable reallocates the Refs vector that the scan
  /// reads (under this same HeapMutex) for every registered table; the grower
  /// holds the returned lock across the resize so the scan never iterates a
  /// half-reallocated, freed buffer. In-place slot writes (setRefs/fillRefs/
  /// setRefAddr) do not need it: they keep the buffer stable, so their races
  /// are the benign conservative-read kind the scan already tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockTableRoots() noexcept {
    return std::unique_lock<std::mutex>(HeapMutex);
  }

  /// Serialize a value-stack reallocation against the collector's root scan,
  /// which iterates every registered ValueStack (under this same StackMutex).
  /// A push that grows past the reserved capacity reallocates and frees the
  /// buffer the scan is iterating; the pusher holds this lock only on that rare
  /// growth path (StackManager::push/pushSpan check size vs capacity first), so
  /// the steady-state push stays lock-free. In-place writes (emplaceTop) and
  /// non-reallocating pushes keep the buffer stable -- their races are the
  /// benign conservative-read kind the scan already tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockStackRoots() noexcept {
    return std::unique_lock<std::mutex>(StackMutex);
  }

private:
  // Mark every registered root (value stacks, tables, globals, element
  // segments, exception payloads, host roots, and -- when ScanNativeStack is
  // set -- the current thread's native stack) gray. Shared by manualCollect and
  // autoCollect, which only differ in how they enter the MarkingRoot phase.
  void markRoots(bool ScanNativeStack) noexcept;

  // Spill the callee-saved register file onto the native stack, then mark every
  // word of the current thread's active stack as a conservative root. Must run
  // in a frame that stays alive across the (synchronous) root scan; see the
  // definition for why the spill cannot live inside getStack().
  void markNativeStackRoots() noexcept;

  // Active native stack span [Frame, stack base). Frame must point into a live
  // frame (typically the address of a local in the caller) so the scan covers
  // the spilled registers and every caller frame up to the base.
  static Span<uint8_t *const> getStack(void *Frame) noexcept;

  [[nodiscard]] uint8_t *doAllocate(uint32_t N) noexcept;

  void doDeallocate(uint8_t *P, uint32_t Size) noexcept;

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const ValVariant &Val) noexcept {
    // The managed pointer lives in the second 64-bit word: RefVariant (and the
    // RefVariant slot of ValVariant) is a 128-bit value whose low word carries
    // the type tag and whose high word holds the object pointer. If that layout
    // ever changes the GC would silently miss every root, so keep this in sync
    // with RefVariant in common/types.h. Copy the raw bits out via memcpy
    // instead of type-punning the value through an `std::array<uint64_t, 2>`
    // reference, which violates strict aliasing.
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
  // White), retrying if a concurrent Sweep() swap reassigns the Black/White
  // roles between loading the mutex pointer and taking the lock. Once it
  // returns, the locked mutex is guaranteed to still guard the matching set, so
  // the caller can load and mutate that set without racing the swap.
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
  std::atomic<uint64_t> Threshold = 1024 * 1024;
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
