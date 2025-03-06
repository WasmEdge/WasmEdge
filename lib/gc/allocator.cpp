// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "gc/allocator.h"
#include "runtime/instance/module.h"
#include "system/allocator.h"
#include <fmt/ostream.h>

namespace {

[[gnu::no_sanitize_thread]] inline uint8_t *
getPointer(const WasmEdge::ValVariant &Val) noexcept {
  return reinterpret_cast<uint8_t *>(
      reinterpret_cast<const std::array<uint64_t, 2> &>(Val)[1]);
}

[[gnu::no_sanitize_thread]] inline uint8_t *
getPointer(const WasmEdge::RefVariant &Ref) noexcept {
  return reinterpret_cast<uint8_t *>(
      reinterpret_cast<const std::array<uint64_t, 2> &>(Ref)[1]);
}

[[gnu::no_sanitize_thread]] inline uint8_t *
getPointer(uint8_t *const &Ref) noexcept {
  return Ref;
}

} // namespace

FMT_BEGIN_NAMESPACE
template <typename Char>
struct formatter<std::thread::id, Char>
    : formatter<std::thread::native_handle_type, Char> {
  template <typename FormatContext>
  auto format(const std::thread::id &ID, FormatContext &Ctx) const
      -> decltype(Ctx.out()) {
    return formatter<__gthread_t, Char>::format(
        *reinterpret_cast<const std::thread::native_handle_type *>(&ID), Ctx);
  }
};
FMT_END_NAMESPACE

using namespace std::literals;

namespace WasmEdge::GC {

Allocator::Allocator()
    : WorkerCount(std::thread::hardware_concurrency()),
      Collectors(std::thread::hardware_concurrency()) {
  for (auto &Collector : Collectors) {
    Collector = std::thread([this] {
      auto Sweep = [this]() {
        EnableWriteBarrier.store(false, std::memory_order_release);
        // spdlog::error("sweep");
        // deallocate white
        {
          std::chrono::steady_clock::time_point Start =
              std::chrono::steady_clock::now();
          auto &W = *White.load(std::memory_order_relaxed);
          uint64_t Freed = 0;
          for (auto *H : W) {
            // spdlog::error("white {} size:{} g:{}", fmt::ptr(H), H->Size,
            //               H->IsGray.load());
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
            spdlog::error("GC: Freed {} bytes in {} ms"sv, Freed,
                          Milli.count());
          }
        }

        // swap black and white
        // spdlog::error("swap {}:{} {}:{}",
        //               fmt::ptr(Black.load(std::memory_order_relaxed)),
        //               Black.load(std::memory_order_relaxed)->size(),
        //               fmt::ptr(White.load(std::memory_order_relaxed)),
        //               White.load(std::memory_order_relaxed)->size());
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
        EnableWriteBarrier.store(true, std::memory_order_release);
      };

      while (Stop.load(std::memory_order_acquire) == false) {
        Header *H = nullptr;
        {
          std::unique_lock<std::mutex> Locker(GrayMutex);
          if (Gray.empty()) {
            // No gray object, wait for other workers
            if (WorkerCount.fetch_sub(1, std::memory_order_acq_rel) > 1) {
              // spdlog::error("{:x}:wait", std::this_thread::get_id());
              GrayNotEmptyCV.wait_for(Locker, std::chrono::seconds(1));
              // spdlog::error("{:x}:wake", std::this_thread::get_id());
              WorkerCount.fetch_add(1, std::memory_order_acq_rel);
            } else {
              // All other workers are waiting, start sweeping
              WorkerCount.fetch_add(1, std::memory_order_acq_rel);
              Sweep();
              GrayNotEmptyCV.wait(Locker);
            }
            continue;
          }
          H = Gray.front();
          Gray.pop_front();
        }
        // spdlog::error("pop from gray {}", fmt::ptr(H));
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
          // spdlog::error("to_black {} {} size:{} g:{}", fmt::ptr(H),
          //               fmt::ptr(&B), H->Size, H->IsGray.load());
        }
      }
    });
  }
}

Allocator::~Allocator() noexcept {
  // spdlog::error("~Allocator");
  Stop.store(true, std::memory_order_relaxed);
  GrayNotEmptyCV.notify_all();
  for (auto &Collector : Collectors) {
    Collector.join();
  }
  // spdlog::error("Gray:");
  for (auto *H : Gray) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  // spdlog::error("Black:");
  for (auto *H : *Black.load(std::memory_order_relaxed)) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
  // spdlog::error("White:");
  for (auto *H : *White.load(std::memory_order_relaxed)) {
    do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
  }
}

void Allocator::collect(Span<uint8_t *const> Stack) noexcept {
  if (std::chrono::steady_clock::now() <
      NextGC.load(std::memory_order_relaxed)) {
    return;
  }

  if (EnableWriteBarrier.load(std::memory_order_acquire) == true) {
    return;
  }
  // spdlog::error("mark root gray");
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
    for (auto V : Heaps) {
      for (const auto &Ref : *V) {
        markGray(getPointer(Ref));
      }
    }
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    for (auto Val : Globals) {
      markGray(getPointer(*Val));
    }
  }
  EnableWriteBarrier.store(true, std::memory_order_release);
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  GrayNotEmptyCV.notify_all();
}

void Allocator::writeBarrier(uint8_t *Pointer) const noexcept {
  if (!EnableWriteBarrier.load(std::memory_order_acquire)) {
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
  spdlog::error("{:x}:do_allocate({}) = {}", std::this_thread::get_id(), N,
                fmt::ptr(P));
  return P;
}

void Allocator::do_deallocate(uint8_t *P, uint32_t N) noexcept {
  spdlog::error("{:x}:do_deallocate({}) = {}", std::this_thread::get_id(), N,
                fmt::ptr(P));
  Used.fetch_sub(N, std::memory_order_acq_rel);
  std::free(P);
}

void Allocator::addStack(std::vector<ValVariant> &Vector) noexcept {
  std::unique_lock<std::mutex> Locker(StackMutex);
  Stacks.emplace_back(&Vector);
}

void Allocator::removeStack(std::vector<ValVariant> &Vector) noexcept {
  std::unique_lock<std::mutex> Locker(StackMutex);
  auto It = std::find(Stacks.begin(), Stacks.end(), &Vector);
  if (It != Stacks.end()) {
    Stacks.erase(It);
  }
}

void Allocator::addHeap(std::vector<RefVariant> &Vector) noexcept {
  std::unique_lock<std::mutex> Locker(HeapMutex);
  Heaps.emplace_back(&Vector);
}

void Allocator::removeHeap(std::vector<RefVariant> &Vector) noexcept {
  std::unique_lock<std::mutex> Locker(HeapMutex);
  auto It = std::find(Heaps.begin(), Heaps.end(), &Vector);
  if (It != Heaps.end()) {
    Heaps.erase(It);
  }
}

void Allocator::addGlobal(ValVariant &Value) noexcept {
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  Globals.emplace_back(&Value);
}

void Allocator::removeGlobal(ValVariant &Value) noexcept {
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  auto It = std::find(Globals.begin(), Globals.end(), &Value);
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
    // spdlog::error("mark gray {}", fmt::ptr(H));
    assuming(H->IsGray.exchange(true, std::memory_order_acq_rel) == false);
    std::unique_lock<std::mutex> Locker(GrayMutex);
    Gray.push_back(H);
  }
}

} // namespace WasmEdge::GC
