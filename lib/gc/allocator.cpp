// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "gc/allocator.h"
#include "common/spdlog.h"
#include "runtime/instance/gc.h"
#include "runtime/instance/module.h"

#define FMT_CPP_LIB_FILESYSTEM 0
#include <fmt/std.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
// getStack() uses pthread stack APIs; include explicitly, not transitively.
#include <pthread.h>
#endif

// setjmp spills callee-saved registers before conservative scanning
// (markNativeStackRoots).
#include <csetjmp>
// std::align_val_t / std::nothrow for over-aligned GC block allocation.
#include <new>

namespace {

// TSan annotations for conservative scanning's intentional races: the GC reads
// stack/heap/global data concurrently with execution. Safe -- conservatism
// over-retains a stale pointer, never UAF.
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TSAN_ENABLED 1
#endif
#endif
#if defined(__SANITIZE_THREAD__)
#define TSAN_ENABLED 1
#endif

#ifdef TSAN_ENABLED
extern "C" {
void AnnotateIgnoreReadsBegin(const char *file, int line);
void AnnotateIgnoreReadsEnd(const char *file, int line);
}
#define TSAN_IGNORE_READS_BEGIN() AnnotateIgnoreReadsBegin(__FILE__, __LINE__)
#define TSAN_IGNORE_READS_END() AnnotateIgnoreReadsEnd(__FILE__, __LINE__)
#else
#define TSAN_IGNORE_READS_BEGIN()
#define TSAN_IGNORE_READS_END()
#endif

// hardware_concurrency() may return 0; clamp to 1, else no worker drives
// marking/sweeping and manualCollect() hangs forever.
uint32_t collectorThreadCount() noexcept {
  const uint32_t N = std::thread::hardware_concurrency();
  return N == 0 ? UINT32_C(1) : N;
}

} // namespace

using namespace std::literals;

namespace WasmEdge {
namespace GC {

Allocator::Allocator() {
  // WorkerCount and Collectors.size() must agree exactly for the
  // last-worker-detects-sweep logic below.
  const uint32_t Workers = collectorThreadCount();
  WorkerCount.store(Workers, std::memory_order_relaxed);
  Collectors.reserve(Workers);
  // Worker body, defined once so the spawn loop can wrap it in try/catch.
  auto WorkerBody = [this] {
    auto Sweep = [this]() -> bool {
      // CAS-gate the sweep to the MarkingGray phase this worker observed.
      // Without it, a racing collect() that starts a new cycle (->MarkingRoot)
      // plus a spurious GCCV wakeup could re-enter Sweep() during the new root
      // scan and free the still-white live set (use-after-free).
      GCState Expected = GCState::MarkingGray;
      if (!CurrentGCState.compare_exchange_strong(Expected, GCState::Sweeping,
                                                  std::memory_order_acq_rel)) {
        return false;
      }
      // deallocate white
      {
        std::chrono::steady_clock::time_point Start =
            std::chrono::steady_clock::now();
        // Hold WhiteMutex while iterating/clearing White to not race mutator
        // write barriers, which erase under it (markGray). Sweep holds
        // GrayMutex; markGray never nests Gray/White, so Gray->White can't
        // deadlock.
        std::unique_lock<std::mutex> WLocker(
            *WhiteMutex.load(std::memory_order_relaxed));
        auto &W = *White.load(std::memory_order_relaxed);
        // size() under WhiteMutex: a concurrent barrier erase would otherwise
        // data-race.
        spdlog::debug("worker sweep W:{}"sv, W.size());
        uint64_t Freed = 0;
        for (auto *H : W) {
          Freed += H->Size;
          doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
        }
        W.clear();
        std::chrono::steady_clock::time_point End =
            std::chrono::steady_clock::now();
        if (Freed > 0) {
          const auto Duration = End - Start;
          const auto Milli =
              std::chrono::duration_cast<std::chrono::milliseconds>(Duration);
          spdlog::debug("GC: Freed {} bytes in {} ms"sv, Freed, Milli.count());
        }
      }

      // Swap black and white. Log set pointers, not size(): reading the sets
      // unlocked would race markGray().
      spdlog::debug("swap B:{} W:{}"sv,
                    fmt::ptr(Black.load(std::memory_order_relaxed)),
                    fmt::ptr(White.load(std::memory_order_relaxed)));
      {
        // Hold BOTH set mutexes around the role swap. markGray (via
        // lockSetMutex) loads the mutex pointer then the set pointer in two
        // steps; without both held the swap could land between them, leaving
        // markGray mutating a set under the wrong mutex (corruption). Both held
        // serializes the swap against every validated lock.
        std::scoped_lock SwapLock(Set1Mutex, Set2Mutex);
        std::mutex *BM = BlackMutex.load(std::memory_order_relaxed);
        std::mutex *WM = WhiteMutex.load(std::memory_order_relaxed);
        std::unordered_set<Header *> *B = Black.load(std::memory_order_relaxed);
        std::unordered_set<Header *> *W = White.load(std::memory_order_relaxed);
        BlackMutex.store(WM, std::memory_order_release);
        WhiteMutex.store(BM, std::memory_order_release);
        Black.store(W, std::memory_order_release);
        White.store(B, std::memory_order_release);
      }
      CurrentGCState.store(GCState::Idle, std::memory_order_release);
      return true;
    };

    while (Stop.load(std::memory_order_acquire) == false) {
      Header *H = nullptr;
      {
        std::unique_lock<std::mutex> Locker(GrayMutex);
        if (Gray.empty()) {
          // No gray object, wait for other workers
          if (WorkerCount.fetch_sub(1, std::memory_order_acq_rel) > 1) {
            GrayNotEmptyCV.wait(Locker, [this] {
              return !Gray.empty() || Stop.load(std::memory_order_acquire);
            });
            WorkerCount.fetch_add(1, std::memory_order_acq_rel);
          } else {
            // All other workers are waiting, start sweeping
            WorkerCount.fetch_add(1, std::memory_order_acq_rel);
            Sweep();
            Locker.unlock();
            std::unique_lock<std::mutex> Locker2(GCMutex);
            GCCV.notify_all();
            // Resume only when a new cycle publishes MarkingGray: waking on its
            // MarkingRoot would race the root scan (see the Sweep CAS gate).
            GCCV.wait(Locker2, [this] {
              return CurrentGCState.load(std::memory_order_acquire) ==
                         GCState::MarkingGray ||
                     Stop.load(std::memory_order_acquire);
            });
            GrayNotEmptyCV.notify_all();
          }
          continue;
        }
        H = Gray.front();
        Gray.pop_front();
      }
      assuming(H->IsGray.load(std::memory_order_relaxed));
      // Mark H's children gray
      const auto *Raw =
          reinterpret_cast<const Runtime::Instance::GCInstance::RawData *>(
              reinterpret_cast<uint8_t *>(H) + sizeof(Header));
      Span<const ValVariant> Pointers(Raw->data(), Raw->Length);
      for (size_t I = 0; I < Pointers.size(); ++I) {
        markGray(getPointer(Pointers[I]));
      }
      H->IsGray.store(false, std::memory_order_relaxed);
      {
        // Validated lock, mirroring markGray's White access. The swap only runs
        // when this is the last marking worker, but pairing the mutex/set loads
        // stays correct without relying on that.
        auto Locker = lockSetMutex(BlackMutex);
        Black.load(std::memory_order_acquire)->emplace(H);
      }
    }
  };
  // Spawn workers. If a std::thread ctor throws (OS refuses a thread), the
  // already-created joinable threads would std::terminate at destruction; stop
  // and join them, then rethrow.
// MSVC signals exceptions via _CPPUNWIND, not __EXCEPTIONS; guarding on the
// latter alone would drop this cleanup on MSVC.
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    for (uint32_t I = 0; I < Workers; ++I) {
      Collectors.emplace_back(WorkerBody);
    }
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch (...) {
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      Stop.store(true, std::memory_order_release);
    }
    GrayNotEmptyCV.notify_all();
    GCCV.notify_all();
    for (auto &Collector : Collectors) {
      if (Collector.joinable()) {
        Collector.join();
      }
    }
    throw;
  }
#endif
}

Allocator::~Allocator() noexcept {
  // Publish Stop under each CV's mutex so a worker that checked its predicate
  // but hasn't parked can't miss the wakeup and hang join().
  {
    std::unique_lock<std::mutex> Locker(GrayMutex);
    Stop.store(true, std::memory_order_release);
  }
  GrayNotEmptyCV.notify_all();
  {
    std::unique_lock<std::mutex> Locker(GCMutex);
  }
  GCCV.notify_all();
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    for (auto T : Heaps) {
      T->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    for (auto G : Globals) {
      G->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(ElemMutex);
    for (auto E : Elems) {
      E->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(ExceptionMutex);
    for (auto Ex : Exceptions) {
      Ex->clearAllocator(*this);
    }
  }
  for (auto &Collector : Collectors) {
    Collector.join();
  }
  for (auto *H : Gray) {
    doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  for (auto *H : *Black.load(std::memory_order_relaxed)) {
    doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  for (auto *H : *White.load(std::memory_order_relaxed)) {
    doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
}

void Allocator::markRoots(bool ScanNativeStack) noexcept {
  // KNOWN LIMITATION (multi-threaded AOT): native-stack scanning covers only
  // the calling thread (markNativeStackRoots uses current-thread APIs) and
  // marking is not stop-the-world. If another thread runs AOT code holding a GC
  // ref only in a register/stack slot during this collect, that ref is on no
  // root set and can be reclaimed. Safe today: the interpreter keeps every live
  // ref on a tracked value stack and never requests native scanning. Concurrent
  // AOT on one allocator would need all-thread scanning or stop-the-world.
  //
  // The same single-mutator assumption underlies every write barrier
  // (GlobalInstance::setValue, TableInstance::setRefAddr/setRefs/fillRefs, AOT
  // global.set/struct.new/array.new): each shades around a store while gated on
  // a non-Idle state that only the storing thread leaves Idle (autoCollect/
  // manualCollect run Idle->MarkingRoot and the synchronous root scan on the
  // mutator). One mutator can't scan roots between a shade and its store; a
  // *second* mutator flipping the state mid-store could, scanning past the
  // not-yet-stored value while the shade was a no-op -- the same limitation,
  // not a separate barrier-ordering bug.
  if (ScanNativeStack) {
    markNativeStackRoots();
  }
  {
    std::unique_lock<std::mutex> Locker(StackMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto V : Stacks) {
      for (const auto &Val : *V) {
        markGray(getPointer(Val));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto T : Heaps) {
      // InitValue holds a GC ref for growTable's slot fill but isn't in Refs;
      // scan it explicitly, else a one-arg grow broadcasts a dangling ref into
      // new slots (use-after-free).
      markGray(getPointer(T->InitValue));
      for (const auto &Ref : T->Refs) {
        markGray(getPointer(Ref));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto G : Globals) {
      markGray(getPointer(G->Value));
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(ElemMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto E : Elems) {
      for (const auto &Ref : E->getRefs()) {
        markGray(getPointer(Ref));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(ExceptionMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto Ex : Exceptions) {
      for (const auto &Val : Ex->getPayload()) {
        markGray(getPointer(Val));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(HostRootsMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (const auto &Ptr : HostRoots) {
      markGray(Ptr);
    }
    TSAN_IGNORE_READS_END();
  }
}

bool Allocator::manualCollect(bool ScanNativeStack) noexcept {
  GCState State = GCState::Idle;
  if (!CurrentGCState.compare_exchange_strong(State, GCState::MarkingRoot,
                                              std::memory_order_acq_rel)) {
    return false;
  }
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  markRoots(ScanNativeStack);
  CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
  std::unique_lock<std::mutex> Locker(GCMutex);
  GCCV.notify_all();
  GCCV.wait(Locker, [this] {
    return CurrentGCState.load(std::memory_order_acquire) == GCState::Idle;
  });
  return true;
}

void Allocator::autoCollect(bool ScanNativeStack) noexcept {
  if (unlikely(EnableManualGC.load(std::memory_order_acquire) == true)) {
    return;
  }
  if (std::chrono::steady_clock::now() <
      NextGC.load(std::memory_order_relaxed)) {
    return;
  }
  GCState State = GCState::Idle;
  if (!CurrentGCState.compare_exchange_strong(State, GCState::MarkingRoot,
                                              std::memory_order_acq_rel)) {
    return;
  }
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  markRoots(ScanNativeStack);
  {
    // Publish MarkingGray and notify with GCMutex held so a worker parked on
    // GCCV can't miss the wakeup and stall the cycle. (manualCollect publishes
    // before taking the lock.)
    std::unique_lock<std::mutex> Locker(GCMutex);
    CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
    GCCV.notify_all();
  }
}

void Allocator::doWriteBarrier(uint8_t *Target) noexcept { markGray(Target); }

std::unique_lock<std::mutex>
Allocator::lockSetMutex(std::atomic<std::mutex *> &MutexPtr) noexcept {
  // Load the mutex pointer, lock, re-check: if a Sweep() swap reassigned the
  // role meanwhile, retry with the new mutex. The swap holds both set mutexes,
  // so once the re-check passes the paired set pointer is stable for the lock's
  // lifetime. Swaps happen at most once per cycle, so the retry rarely spins.
  for (;;) {
    std::mutex *M = MutexPtr.load(std::memory_order_acquire);
    std::unique_lock<std::mutex> Locker(*M);
    if (M == MutexPtr.load(std::memory_order_acquire)) {
      return Locker;
    }
  }
}

[[nodiscard]] uint8_t *Allocator::doAllocate(uint32_t N) noexcept {
  // Reserve N via CAS, rejecting when Old + N exceeds Threshold. A plain
  // fetch_add tests pre-increment Used and lets one large N overshoot; summing
  // first keeps a hard bound. Old (uint64_t) + N (uint32_t) can't wrap.
  const uint64_t Limit = Threshold.load(std::memory_order_relaxed);
  uint64_t Old = Used.load(std::memory_order_relaxed);
  do {
    if (Old + N > Limit) {
      return nullptr;
    }
  } while (!Used.compare_exchange_weak(Old, Old + N, std::memory_order_acq_rel,
                                       std::memory_order_relaxed));
  // Over-align to the header (16): RawData embeds ValVariant whose SIMD members
  // need 16-byte alignment, but malloc guarantees only max_align_t (8 on some
  // 32-bit ABIs) -- misaligned payload is UB / SIGBUS on vector loads.
  uint8_t *P = static_cast<uint8_t *>(
      ::operator new(N, std::align_val_t{alignof(Header)}, std::nothrow));
  spdlog::debug("{} allocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  if (unlikely(P == nullptr)) {
    // Failed: release the reserved bytes.
    Used.fetch_sub(N, std::memory_order_acq_rel);
  }
  return P;
}

void Allocator::doDeallocate(uint8_t *P, uint32_t N) noexcept {
  spdlog::debug("{} deallocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  Used.fetch_sub(N, std::memory_order_acq_rel);
  ::operator delete(P, std::align_val_t{alignof(Header)});
}

void Allocator::addStack(std::vector<ValVariant> &Vector) noexcept {
  std::unique_lock<std::mutex> Locker(StackMutex);
  Stacks.emplace_back(&Vector);
}

void Allocator::removeStack(std::vector<ValVariant> &Vector) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(StackMutex);
  auto It = std::find(Stacks.begin(), Stacks.end(), &Vector);
  if (It != Stacks.end()) {
    Stacks.erase(It);
  }
}

void Allocator::addTable(Runtime::Instance::TableInstance &Table) noexcept {
  std::unique_lock<std::mutex> Locker(HeapMutex);
  Heaps.emplace_back(&Table);
}

void Allocator::removeTable(Runtime::Instance::TableInstance &Table) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HeapMutex);
  auto It = std::find(Heaps.begin(), Heaps.end(), &Table);
  if (It != Heaps.end()) {
    Heaps.erase(It);
  }
}

void Allocator::addGlobal(Runtime::Instance::GlobalInstance &Global) noexcept {
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  Globals.emplace_back(&Global);
}

void Allocator::removeGlobal(
    Runtime::Instance::GlobalInstance &Global) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  auto It = std::find(Globals.begin(), Globals.end(), &Global);
  if (It != Globals.end()) {
    Globals.erase(It);
  }
}

void Allocator::addElem(Runtime::Instance::ElementInstance &Elem) noexcept {
  std::unique_lock<std::mutex> Locker(ElemMutex);
  Elems.emplace_back(&Elem);
}

void Allocator::removeElem(Runtime::Instance::ElementInstance &Elem) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ElemMutex);
  auto It = std::find(Elems.begin(), Elems.end(), &Elem);
  if (It != Elems.end()) {
    Elems.erase(It);
  }
}

void Allocator::addException(
    Runtime::Instance::ExceptionInstance &Exception) noexcept {
  std::unique_lock<std::mutex> Locker(ExceptionMutex);
  Exceptions.emplace_back(&Exception);
}

void Allocator::removeException(
    Runtime::Instance::ExceptionInstance &Exception) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ExceptionMutex);
  auto It = std::find(Exceptions.begin(), Exceptions.end(), &Exception);
  if (It != Exceptions.end()) {
    Exceptions.erase(It);
  }
}

void Allocator::retainResult(const RefVariant &Ref) noexcept {
  // Like releaseRef/releaseAllRefs: skip during teardown so we don't append to
  // a host-root bag about to be destroyed.
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  HostRoots.emplace_back(Ref.getPtr<uint8_t>());
}

WASMEDGE_EXPORT void Allocator::releaseRef(const RefVariant &Ref) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  // Scan from the back: retainResult appends, so LIFO matches quickly. Remove
  // via swap-with-back + pop_back -- HostRoots is an unordered bag matched by
  // pointer identity, so order and which instance don't matter.
  auto It =
      std::find(HostRoots.rbegin(), HostRoots.rend(), Ref.getPtr<uint8_t>());
  if (It != HostRoots.rend()) {
    *It = HostRoots.back();
    HostRoots.pop_back();
  }
}

WASMEDGE_EXPORT void Allocator::releaseAllRefs() noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  HostRoots.clear();
}

void Allocator::markGray(uint8_t *Pointer) noexcept {
  if (const auto Address = reinterpret_cast<uintptr_t>(Pointer);
      Address <= sizeof(Header) || Address % alignof(Header) != 0) {
    return;
  }
  Header *H = reinterpret_cast<Header *>(Pointer - sizeof(Header));
  // Hold GrayMutex across BOTH the White-erase and Gray-push so the shade is
  // atomic w.r.t. Sweep() termination. The last worker sweeps only after seeing
  // Gray empty under GrayMutex, and Sweep frees White under GrayMutex->
  // WhiteMutex; same order here stops a sweep interposing between erase and
  // push. Otherwise H is briefly in NEITHER set and could be swept -- freeing
  // H's still-white children (reachable only through H), leaving dangling
  // pointers when H is later traced (use-after-free).
  std::unique_lock<std::mutex> GrayLocker(GrayMutex);
  bool IsWhite = false;
  {
    // Validated lock: lockSetMutex retries until the locked mutex still guards
    // White. (No swap runs while we hold GrayMutex -- Sweep holds it throughout
    // -- but the validated load is correct and cheap.)
    auto Locker = lockSetMutex(WhiteMutex);
    IsWhite = White.load(std::memory_order_acquire)->erase(H) > 0;
  }
  if (IsWhite) {
    // Hoist the exchange out of assuming(): marking gray must run in release
    // builds. assuming() evaluates its arg today, but a switch to an
    // unevaluated __builtin_assume would silently drop it.
    const bool WasGray = H->IsGray.exchange(true, std::memory_order_acq_rel);
    assuming(!WasGray);
    Gray.push_back(H);
    // Wake a worker blocked on an empty Gray queue so concurrently-shaded
    // objects are marked this cycle. Guarded to MarkingGray: the root scan
    // shades before that state is published and the driver wakes workers on the
    // transition, so notifying during root marking is only spurious.
    if (CurrentGCState.load(std::memory_order_acquire) ==
        GCState::MarkingGray) {
      GrayNotEmptyCV.notify_one();
    }
  }
}

WASMEDGE_GC_DISABLE_SANITIZER
void Allocator::markNativeStackRoots() noexcept {
  // setjmp spills callee-saved registers into Buf in this frame; getStack(&Buf)
  // anchors the scanned span at Buf so the spill is covered, and the scan runs
  // while this frame is alive. Catches a GC ref an AOT caller keeps only in a
  // callee-saved register across an alloc intrinsic -- on no root set, it would
  // otherwise be swept and dangle. Caller-saved registers need no spill. The
  // spill can't live in getStack() -- its frame is popped before the scan.
  std::jmp_buf Buf;
  (void)setjmp(Buf);
  // Conservative scan: intentionally reads data execution threads may be
  // modifying. Suppress TSan.
  TSAN_IGNORE_READS_BEGIN();
  for (const auto &Val : getStack(&Buf)) {
    markGray(getPointer(Val));
  }
  TSAN_IGNORE_READS_END();
}

Span<uint8_t *const> Allocator::getStack(void *Frame) noexcept {
  // Frame is the low end of the active stack (an address in a live frame), so
  // the span covers the spill and every caller frame up to the base. The stack
  // grows down, so the base is the high address; the region below Frame holds
  // no roots and is excluded (scanning it wastes time, over-retains, and faults
  // untouched pages).
  uintptr_t StackEnd = reinterpret_cast<uintptr_t>(Frame);
#if WASMEDGE_OS_LINUX
  pthread_attr_t Attr;
  int Error = pthread_getattr_np(pthread_self(), &Attr);
  if (likely(!Error)) {
    void *StackBegin;
    size_t StackSize;
    Error = pthread_attr_getstack(&Attr, &StackBegin, &StackSize);
    pthread_attr_destroy(&Attr);
    if (likely(Error == 0)) {
      uintptr_t StackBase = reinterpret_cast<uintptr_t>(StackBegin);
      uintptr_t StackTop = StackBase + StackSize;
      // Reject a frame outside [StackBase, StackTop]: the span length is the
      // unsigned (StackTop - StackEnd), which would underflow to a near-
      // SIZE_MAX count and scan out of bounds. Fall through to the error path.
      if (likely(StackEnd >= StackBase && StackEnd <= StackTop)) {
        return Span<uint8_t *const>{
            reinterpret_cast<uint8_t *const *>(StackEnd),
            (StackTop - StackEnd) / sizeof(uint8_t *)};
      }
    }
  }
  // Could not read native stack bounds: an empty span means no native-stack
  // roots this cycle, so a ref held only by AOT code in a register/stack slot
  // could be swept. Should never happen for a live thread; surface it.
  spdlog::error("GC: failed to read native stack bounds; native-stack roots "
                "will not be scanned this cycle"sv);
  return {};

#elif WASMEDGE_OS_MACOS
  uintptr_t StackBegin =
      reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
  // Reject a base not above the captured frame: (StackBegin - StackEnd) is
  // unsigned and would otherwise underflow.
  if (unlikely(StackEnd > StackBegin)) {
    spdlog::error("GC: native stack base below current frame; native-stack "
                  "roots will not be scanned this cycle"sv);
    return {};
  }
  return Span<uint8_t *const>{reinterpret_cast<uint8_t *const *>(StackEnd),
                              (StackBegin - StackEnd) / sizeof(uint8_t *)};

#elif WASMEDGE_OS_WINDOWS
#if defined(_M_X64) || defined(_M_IX86)
  uintptr_t StackBegin = reinterpret_cast<uintptr_t>(
      reinterpret_cast<winapi::NT_TIB_ *>(winapi::NtCurrentTeb())->StackBase);
#elif defined(_M_ARM64)
  winapi::ULONG_PTR_ LowLimit, HighLimit;
  winapi::GetCurrentThreadStackLimits(&LowLimit, &HighLimit);
  uintptr_t StackBegin = reinterpret_cast<uintptr_t>(HighLimit);
#else
#error Unsupported architecture for Windows
#endif
  // Reject a base not above the captured frame: (StackBegin - StackEnd) is
  // unsigned and would otherwise underflow.
  if (unlikely(StackEnd > StackBegin)) {
    spdlog::error("GC: native stack base below current frame; native-stack "
                  "roots will not be scanned this cycle"sv);
    return {};
  }
  return Span<uint8_t *const>{reinterpret_cast<uint8_t *const *>(StackEnd),
                              (StackBegin - StackEnd) / sizeof(uint8_t *)};

#else
#error Unsupported architecture
#endif
}

} // namespace GC
} // namespace WasmEdge
