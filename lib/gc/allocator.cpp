// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "gc/allocator.h"
#include "runtime/instance/module.h"
#include "system/allocator.h"

#define FMT_CPP_LIB_FILESYSTEM 0
#include <fmt/std.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

namespace {

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#define DISABLE_SANITIZER
#else
#define DISABLE_SANITIZER [[gnu::no_sanitize_address, gnu::no_sanitize_thread]]
#endif
DISABLE_SANITIZER inline uint8_t *
getPointer(const WasmEdge::ValVariant &Val) noexcept {
  return reinterpret_cast<uint8_t *>(
      reinterpret_cast<const std::array<uint64_t, 2> &>(Val)[1]);
}

DISABLE_SANITIZER inline uint8_t *
getPointer(const WasmEdge::RefVariant &Ref) noexcept {
  return reinterpret_cast<uint8_t *>(
      reinterpret_cast<const std::array<uint64_t, 2> &>(Ref)[1]);
}

DISABLE_SANITIZER inline uint8_t *getPointer(uint8_t *const &Ref) noexcept {
  return Ref;
}
#undef DISABLE_SANITIZER

} // namespace

using namespace std::literals;

namespace WasmEdge::GC {

Allocator::Allocator()
    : WorkerCount(std::thread::hardware_concurrency()),
      Collectors(std::thread::hardware_concurrency()) {
  for (auto &Collector : Collectors) {
    Collector = std::thread([this] {
      auto Sweep = [this]() {
        CurrentGCState.store(GCState::Sweeping, std::memory_order_release);
        spdlog::debug("worker sweep W:{} B:{}"sv,
                      White.load(std::memory_order_relaxed)->size(),
                      Black.load(std::memory_order_relaxed)->size());
        // deallocate white
        {
          std::chrono::steady_clock::time_point Start =
              std::chrono::steady_clock::now();
          auto &W = *White.load(std::memory_order_relaxed);
          uint64_t Freed = 0;
          for (auto *H : W) {
            spdlog::debug("{} in white size:{} g:{}"sv, fmt::ptr(H), H->Size,
                          H->IsGray.load());
            Freed += H->Size;
            do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
          }
          W.clear();
          std::chrono::steady_clock::time_point End =
              std::chrono::steady_clock::now();
          if (Freed > 0) {
            const auto Duration = End - Start;
            const auto Milli =
                std::chrono::duration_cast<std::chrono::milliseconds>(Duration);
            spdlog::debug("GC: Freed {} bytes in {} ms"sv, Freed,
                          Milli.count());
          }
        }

        // swap black and white
        spdlog::debug("swap B:{}:{} W:{}:{}"sv,
                      fmt::ptr(Black.load(std::memory_order_relaxed)),
                      Black.load(std::memory_order_relaxed)->size(),
                      fmt::ptr(White.load(std::memory_order_relaxed)),
                      White.load(std::memory_order_relaxed)->size());
        {
          std::mutex *BM = BlackMutex.load(std::memory_order_relaxed);
          std::mutex *WM = WhiteMutex.load(std::memory_order_relaxed);
          std::unordered_set<Header *> *B =
              Black.load(std::memory_order_relaxed);
          std::unordered_set<Header *> *W =
              White.load(std::memory_order_relaxed);
          BlackMutex.store(WM, std::memory_order_relaxed);
          WhiteMutex.store(BM, std::memory_order_relaxed);
          Black.store(W, std::memory_order_relaxed);
          White.store(B, std::memory_order_relaxed);
        }
        CurrentGCState.store(GCState::Idle, std::memory_order_release);
      };

      while (Stop.load(std::memory_order_acquire) == false) {
        Header *H = nullptr;
        {
          std::unique_lock<std::mutex> Locker(GrayMutex);
          if (Gray.empty()) {
            // No gray object, wait for other workers
            if (WorkerCount.fetch_sub(1, std::memory_order_acq_rel) > 1) {
              GrayNotEmptyCV.wait(Locker);
              WorkerCount.fetch_add(1, std::memory_order_acq_rel);
            } else {
              // All other workers are waiting, start sweeping
              WorkerCount.fetch_add(1, std::memory_order_acq_rel);
              Sweep();
              Locker.unlock();
              std::unique_lock<std::mutex> Locker2(GCMutex);
              GCCV.notify_all();
              GCCV.wait(Locker2);
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
          std::unique_lock<std::mutex> Locker(
              *BlackMutex.load(std::memory_order_relaxed));
          auto &B = *Black.load(std::memory_order_relaxed);
          B.emplace(H);
        }
      }
    });
  }
}

Allocator::~Allocator() noexcept {
  Stop.store(true, std::memory_order_relaxed);
  GrayNotEmptyCV.notify_all();
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
  for (auto &Collector : Collectors) {
    Collector.join();
  }
  for (auto *H : Gray) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  for (auto *H : *Black.load(std::memory_order_relaxed)) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  for (auto *H : *White.load(std::memory_order_relaxed)) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
}

bool Allocator::manualCollect(Span<uint8_t *const> Stack) noexcept {
  GCState State = GCState::Idle;
  if (!CurrentGCState.compare_exchange_strong(State, GCState::MarkingRoot,
                                              std::memory_order_acq_rel)) {
    return false;
  }
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  for (const auto &Val : Stack) {
    markGray(getPointer(Val));
  }
  {
    std::unique_lock<std::mutex> Locker(StackMutex);
    for (auto V : Stacks) {
      for (const auto &Val : *V) {
        markGray(getPointer(Val));
      }
    }
  }
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    for (auto T : Heaps) {
      for (const auto &Ref : T->Refs) {
        markGray(getPointer(Ref));
      }
    }
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    for (auto G : Globals) {
      markGray(getPointer(G->Value));
    }
  }
  CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
  std::unique_lock<std::mutex> Locker(GCMutex);
  GCCV.notify_all();
  GCCV.wait(Locker);
  return true;
}

void Allocator::autoCollect(Span<uint8_t *const> Stack) noexcept {
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
  // mark root gray
  for (const auto &Val : Stack) {
    markGray(getPointer(Val));
  }
  {
    std::unique_lock<std::mutex> Locker(StackMutex);
    for (auto V : Stacks) {
      for (const auto &Val : *V) {
        markGray(getPointer(Val));
      }
    }
  }
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    for (auto T : Heaps) {
      for (const auto &Ref : T->Refs) {
        markGray(getPointer(Ref));
      }
    }
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    for (auto G : Globals) {
      markGray(getPointer(G->Value));
    }
  }
  CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
  GCCV.notify_all();
}

void Allocator::writeBarrier(uint8_t *Pointer) const noexcept {
  if (likely(CurrentGCState.load(std::memory_order_acquire) !=
             GCState::MarkingGray)) {
    return;
  }
  auto H = reinterpret_cast<Header *>(Pointer - sizeof(Header));
  if (!H->IsGray.exchange(true, std::memory_order_acq_rel)) {
    bool IsWhite = false;
    {
      std::unique_lock<std::mutex> Locker(
          *WhiteMutex.load(std::memory_order_relaxed));
      IsWhite = White.load(std::memory_order_relaxed)->erase(H) > 0;
    }
    if (IsWhite) {
      std::unique_lock<std::mutex> Locker(
          const_cast<Allocator *>(this)->GrayMutex);
      const_cast<Allocator *>(this)->Gray.push_back(H);
    } else {
      H->IsGray.store(false, std::memory_order_release);
    }
  }
}

[[nodiscard]] uint8_t *Allocator::do_allocate(uint32_t N) noexcept {
  if (Used.fetch_add(N, std::memory_order_acq_rel) >
      Threshold.load(std::memory_order_relaxed)) {
    return nullptr;
  }
  uint8_t *P = static_cast<uint8_t *>(std::malloc(N));
  spdlog::debug("{} allocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  return P;
}

void Allocator::do_deallocate(uint8_t *P, uint32_t N) noexcept {
  spdlog::debug("{} deallocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  Used.fetch_sub(N, std::memory_order_acq_rel);
  std::free(P);
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

void Allocator::markGray(uint8_t *Pointer) noexcept {
  if (const auto Address = reinterpret_cast<uintptr_t>(Pointer);
      Address <= sizeof(Header) || Address % alignof(Header) != 0) {
    return;
  }
  Header *H = reinterpret_cast<Header *>(Pointer - sizeof(Header));
  bool IsWhite = false;
  {
    std::unique_lock<std::mutex> Locker(
        *WhiteMutex.load(std::memory_order_relaxed));
    IsWhite = White.load(std::memory_order_relaxed)->erase(H) > 0;
  }
  if (IsWhite) {
    assuming(H->IsGray.exchange(true, std::memory_order_acq_rel) == false);
    std::unique_lock<std::mutex> Locker(GrayMutex);
    Gray.push_back(H);
  }
}

Span<uint8_t *const> Allocator::getStack() noexcept {
#if WASMEDGE_OS_LINUX
  pthread_attr_t Attr;
  int Error = pthread_getattr_np(pthread_self(), &Attr);
  if (likely(!Error)) {
    void *StackBegin;
    size_t StackSize;
    Error = pthread_attr_getstack(&Attr, &StackBegin, &StackSize);
    pthread_attr_destroy(&Attr);
    if (likely(Error == 0)) {
      return Span<uint8_t *const>{
          reinterpret_cast<uint8_t *const *>(StackBegin),
          StackSize / sizeof(uint8_t *)};
    }
  }
  return {};

#elif WASMEDGE_OS_MACOS
  uintptr_t StackEnd = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
  uintptr_t StackBegin =
      reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
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
  uintptr_t StackEnd =
      reinterpret_cast<uintptr_t>(winapi::_AddressOfReturnAddress());
  return Span<uint8_t *const>{reinterpret_cast<uint8_t *const *>(StackEnd),
                              (StackBegin - StackEnd) / sizeof(uint8_t *)};

#else
#error Unsupported architecture
#endif
}

} // namespace WasmEdge::GC
