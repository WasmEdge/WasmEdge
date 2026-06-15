// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "gc/allocator.h"
#include "common/spdlog.h"
#include "runtime/instance/gc.h"
#include "runtime/instance/module.h"

#define FMT_CPP_LIB_FILESYSTEM 0
#include <fmt/std.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
// getStack() calls pthread_self / pthread_getattr_np / pthread_attr_getstack /
// pthread_get_stackaddr_np; include the header explicitly instead of relying on
// transitive includes.
#include <pthread.h>
#endif

// setjmp/jmp_buf: used to spill the callee-saved register file onto the stack
// before conservative scanning (see Allocator::markNativeStackRoots).
#include <csetjmp>
// std::align_val_t / std::nothrow for over-aligned GC block allocation.
#include <new>

namespace {

// TSan annotations for intentional data races in conservative GC scanning.
// The GC reads stack/heap/global data concurrently with execution threads.
// This is safe because the GC is conservative — stale pointers only cause
// extra retention, never use-after-free.
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

// std::thread::hardware_concurrency() may return 0 when the value is not
// computable. Clamp to at least one collector thread, otherwise the GC has no
// worker to drive marking/sweeping and manualCollect() would wait forever.
uint32_t collectorThreadCount() noexcept {
  const uint32_t N = std::thread::hardware_concurrency();
  return N == 0 ? UINT32_C(1) : N;
}

} // namespace

using namespace std::literals;

namespace WasmEdge::GC {

Allocator::Allocator() {
  // Compute the worker count once: WorkerCount and Collectors.size() must agree
  // exactly for the last-worker-detects-sweep logic in the worker loop below.
  const uint32_t Workers = collectorThreadCount();
  WorkerCount.store(Workers, std::memory_order_relaxed);
  Collectors.reserve(Workers);
  // The worker body. Defined once so the spawn loop below can be wrapped in a
  // try/catch for exception safety without duplicating it.
  auto WorkerBody = [this] {
    auto Sweep = [this]() -> bool {
      // Only sweep while still in the MarkingGray phase this worker observed.
      // A racing manualCollect()/autoCollect() can start a new cycle
      // (Idle->MarkingRoot) the instant a prior sweep stored Idle; without
      // this CAS gate a spurious GCCV wakeup could let the last worker
      // re-enter Sweep() during the new cycle's root scan and free the
      // still-white live set (use-after-free).
      GCState Expected = GCState::MarkingGray;
      if (!CurrentGCState.compare_exchange_strong(Expected, GCState::Sweeping,
                                                  std::memory_order_acq_rel)) {
        return false;
      }
      // deallocate white
      {
        std::chrono::steady_clock::time_point Start =
            std::chrono::steady_clock::now();
        // Hold WhiteMutex while iterating and clearing the White set so we do
        // not race with concurrent mutator write barriers, which erase from
        // the same set under WhiteMutex (Allocator::markGray). Sweep() is
        // invoked while holding GrayMutex; markGray only ever holds
        // WhiteMutex and GrayMutex sequentially (never nested), so the
        // Gray->White order taken here introduces no deadlock.
        std::unique_lock<std::mutex> WLocker(
            *WhiteMutex.load(std::memory_order_relaxed));
        auto &W = *White.load(std::memory_order_relaxed);
        // Read the white-set size under WhiteMutex: an in-flight mutator
        // write barrier can erase from this set concurrently, so reading
        // size() without the lock would be a data race.
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

      // swap black and white. Log only the atomic set pointers, not their
      // sizes: size() here would read the unordered_sets without holding
      // their mutexes, racing with concurrent write-barrier markGray().
      spdlog::debug("swap B:{} W:{}"sv,
                    fmt::ptr(Black.load(std::memory_order_relaxed)),
                    fmt::ptr(White.load(std::memory_order_relaxed)));
      {
        // Acquire BOTH concrete set mutexes around the role swap. A
        // concurrent mutator write barrier (markGray, via lockSetMutex) loads
        // the White mutex pointer and then the White set pointer in two
        // steps; without holding both mutexes here the swap could land
        // between those loads and leave markGray erasing from a set it no
        // longer holds the matching mutex for, so two barriers could mutate
        // the same unordered_set under different mutexes (corruption).
        // Holding both serializes the swap against every validated lock.
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
            // Resume only once a new cycle has published MarkingGray (real
            // marking work), not merely on any non-Idle state: waking on the
            // next cycle's MarkingRoot would let this worker race the root
            // scan (see the Sweep() CAS gate above).
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
      // Mark all children of object H as gray
      const auto *Raw =
          reinterpret_cast<const Runtime::Instance::GCInstance::RawData *>(
              reinterpret_cast<uint8_t *>(H) + sizeof(Header));
      Span<const ValVariant> Pointers(Raw->Data, Raw->Length);
      for (size_t I = 0; I < Pointers.size(); ++I) {
        markGray(getPointer(Pointers[I]));
      }
      H->IsGray.store(false, std::memory_order_relaxed);
      {
        // Validated lock, for symmetry with markGray's White access. In
        // practice the swap only runs when this is the last marking worker
        // (so no swap races this insert), but pairing the mutex and set loads
        // keeps the Black access correct without relying on that invariant.
        auto Locker = lockSetMutex(BlackMutex);
        Black.load(std::memory_order_acquire)->emplace(H);
      }
    }
  };
  // Spawn the workers. If a std::thread constructor throws (e.g. the OS refuses
  // a new thread), the partially-built Collectors vector would otherwise be
  // destroyed with joinable threads still running (std::terminate). Stop the
  // already-created workers, join them, then rethrow so the caller sees the
  // failure cleanly.
#ifdef __EXCEPTIONS
  try {
#endif
    for (uint32_t I = 0; I < Workers; ++I) {
      Collectors.emplace_back(WorkerBody);
    }
#ifdef __EXCEPTIONS
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
  // Publish Stop while holding each CV's mutex so a worker that has evaluated
  // its wait predicate (Stop == false) but not yet parked cannot miss the
  // wakeup below and hang Collector.join().
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
  // Conservative native-stack root scanning, for AOT-compiled callers only.
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
      // InitValue holds a GC reference used to fill slots on growTable; it is
      // not in Refs, so it must be scanned explicitly or a one-arg grow can
      // broadcast a dangling reference into the new slots (use-after-free).
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
    // GCCV cannot miss the wakeup and stall the async cycle. (Unlike
    // manualCollect, which publishes the state before taking the lock.)
    std::unique_lock<std::mutex> Locker(GCMutex);
    CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
    GCCV.notify_all();
  }
}

void Allocator::doWriteBarrier(uint8_t *Target) noexcept { markGray(Target); }

std::unique_lock<std::mutex>
Allocator::lockSetMutex(std::atomic<std::mutex *> &MutexPtr) noexcept {
  // Load the current mutex pointer, lock it, then re-check the pointer. If a
  // Sweep() swap reassigned the role while we were taking the lock, the
  // re-check fails and we retry with the new mutex. Because the swap holds both
  // concrete set mutexes while reassigning, once our re-check succeeds the
  // paired set pointer is stable for as long as we hold this lock. Swaps happen
  // at most once per GC cycle, so the retry effectively never spins.
  for (;;) {
    std::mutex *M = MutexPtr.load(std::memory_order_acquire);
    std::unique_lock<std::mutex> Locker(*M);
    if (M == MutexPtr.load(std::memory_order_acquire)) {
      return Locker;
    }
  }
}

[[nodiscard]] uint8_t *Allocator::doAllocate(uint32_t N) noexcept {
  if (Used.fetch_add(N, std::memory_order_acq_rel) >
      Threshold.load(std::memory_order_relaxed)) {
    // Over the threshold: revert the speculative increment so Used keeps
    // tracking live bytes instead of growing unboundedly on every rejection.
    Used.fetch_sub(N, std::memory_order_acq_rel);
    return nullptr;
  }
  // Over-align to the GC header (16): RawData embeds ValVariant, whose SIMD
  // members require 16-byte alignment; plain malloc only guarantees
  // alignof(max_align_t) (8 on some 32-bit ABIs), which would make the payload
  // misaligned (UB / SIGBUS on aligned vector loads).
  uint8_t *P = static_cast<uint8_t *>(
      ::operator new(N, std::align_val_t{alignof(Header)}, std::nothrow));
  spdlog::debug("{} allocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  if (unlikely(P == nullptr)) {
    // Allocation failed: do not leave the reserved bytes counted as used.
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
  // Mirror releaseRef/releaseAllRefs: skip during teardown so we never append
  // to a host-root bag that is about to be destroyed.
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
  // Scan from the back: retainResult appends, so a LIFO release pattern finds
  // its match quickly. Remove via swap-with-back + pop_back -- HostRoots is an
  // unordered bag matched by pointer identity, so order does not matter and any
  // single matching instance is interchangeable.
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
  // Hold GrayMutex across BOTH the White-erase and the Gray-push so the shade
  // is atomic with respect to Sweep() termination. The last worker decides to
  // sweep only after observing Gray empty under GrayMutex, and Sweep() frees
  // the White set under GrayMutex(outer) -> WhiteMutex(inner); taking that same
  // order here means a sweep can never interpose between erasing H from White
  // and pushing it to Gray. Without this, H would briefly be in NEITHER set,
  // and the last worker could observe Gray empty and sweep -- freeing H's
  // still-white children (reachable only through H, not yet traced) -- leaving
  // dangling child pointers when H is finally traced (use-after-free).
  std::unique_lock<std::mutex> GrayLocker(GrayMutex);
  bool IsWhite = false;
  {
    // Validated lock: a concurrent Sweep() swap can reassign the White role
    // between loading the mutex and loading the set. lockSetMutex retries until
    // the locked mutex still guards White, so two barriers can never erase from
    // the same unordered_set under different mutexes. (No swap can run while we
    // hold GrayMutex -- Sweep holds it throughout -- but the validated load is
    // still correct and cheap here.)
    auto Locker = lockSetMutex(WhiteMutex);
    IsWhite = White.load(std::memory_order_acquire)->erase(H) > 0;
  }
  if (IsWhite) {
    // Hoist the exchange out of assuming(): marking the header gray is a
    // required side effect that must run in release builds. assuming() does
    // evaluate its argument today, but a future switch to a truly unevaluated
    // __builtin_assume would silently drop the exchange.
    const bool WasGray = H->IsGray.exchange(true, std::memory_order_acq_rel);
    assuming(!WasGray);
    Gray.push_back(H);
    // Wake a worker blocked on an empty Gray queue so concurrently-shaded
    // objects are marked in the current cycle. Guarded to the marking phase:
    // the root scan shades objects before MarkingGray is published and the
    // driver wakes the workers itself once it transitions, so notifying here
    // during the root phase would only cause spurious wake-ups.
    if (CurrentGCState.load(std::memory_order_acquire) ==
        GCState::MarkingGray) {
      GrayNotEmptyCV.notify_one();
    }
  }
}

WASMEDGE_GC_DISABLE_SANITIZER
void Allocator::markNativeStackRoots() noexcept {
  // setjmp writes the callee-saved register file into Buf, which lives in this
  // frame; getStack(&Buf) then anchors the scanned span at Buf so the spill is
  // included, and the loop below runs while this frame (and Buf) is still
  // alive. This catches a GC reference that an AOT-compiled caller keeps only
  // in a callee-saved register across an allocation intrinsic: it is on no
  // tracked root set, so without the spill the conservative scan would miss it
  // and the object could be swept, leaving a dangling reference in compiled
  // code. Caller-saved registers need no spill -- a caller cannot keep a value
  // live in one across the call, so any live reference is already on the native
  // stack or in a callee-saved register. The spill cannot be done inside
  // getStack(): its frame is popped and reused by the collector before the scan
  // reads it.
  std::jmp_buf Buf;
  (void)setjmp(Buf);
  // Conservative root scanning: intentionally reads data that may be
  // concurrently modified by execution threads. Suppress TSan for these reads.
  TSAN_IGNORE_READS_BEGIN();
  for (const auto &Val : getStack(&Buf)) {
    markGray(getPointer(Val));
  }
  TSAN_IGNORE_READS_END();
}

Span<uint8_t *const> Allocator::getStack(void *Frame) noexcept {
  // Frame is the low end of the active stack: an address in a live frame, so
  // the span covers the spilled registers and every caller frame up to the
  // base. The stack grows down, so the base is the high address; the unused
  // region below Frame holds no live roots and is excluded (scanning it only
  // wastes time, adds false retention, and faults in untouched stack pages).
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
      // Guard against bounds that do not contain the captured frame: the span
      // length is the unsigned difference (StackTop - StackEnd), so a frame
      // outside [StackBase, StackTop] would underflow into a near-SIZE_MAX word
      // count and a massive out-of-bounds scan. Fall through to the error path.
      if (likely(StackEnd >= StackBase && StackEnd <= StackTop)) {
        return Span<uint8_t *const>{
            reinterpret_cast<uint8_t *const *>(StackEnd),
            (StackTop - StackEnd) / sizeof(uint8_t *)};
      }
    }
  }
  // Could not determine the native stack bounds: returning an empty span makes
  // the conservative scan see no native-stack roots this cycle, so a reference
  // held only in a register/stack slot by AOT code could be swept. This should
  // never happen for a live thread; surface it instead of failing silently.
  spdlog::error("GC: failed to read native stack bounds; native-stack roots "
                "will not be scanned this cycle"sv);
  return {};

#elif WASMEDGE_OS_MACOS
  uintptr_t StackBegin =
      reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
  // Guard against a base that does not lie above the captured frame:
  // (StackBegin - StackEnd) is unsigned and would otherwise underflow.
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
      reinterpret_cast<winapi::NT_TIB *>(winapi::NtCurrentTeb())->StackBase);
#elif defined(_M_ARM64)
  winapi::ULONG_PTR_ LowLimit, HighLimit;
  winapi::GetCurrentThreadStackLimits(&LowLimit, &HighLimit);
  uintptr_t StackBegin = reinterpret_cast<uintptr_t>(HighLimit);
#else
#error Unsupported architecture for Windows
#endif
  // Guard against a base that does not lie above the captured frame:
  // (StackBegin - StackEnd) is unsigned and would otherwise underflow.
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

} // namespace WasmEdge::GC
