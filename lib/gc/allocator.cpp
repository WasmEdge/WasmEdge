// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "gc/allocator.h"
#include "runtime/instance/module.h"
#include "system/allocator.h"

using namespace std::literals;

namespace WasmEdge::GC {

Allocator::Allocator()
    : WorkerCount(std::thread::hardware_concurrency()),
      Collectors(std::thread::hardware_concurrency()) {
  for (auto &Collector : Collectors) {
    Collector = std::thread([this] {
      auto IsRef = [](const TypeCode &Type) {
        return Type == TypeCode::ArrayRef || Type == TypeCode::StructRef;
      };
      auto MarkGray = [this](uint8_t *Pointer) {
        Header *H = reinterpret_cast<Header *>(Pointer - sizeof(Header));
        if (!H->IsGray.exchange(true, std::memory_order_acq_rel)) {
          bool IsWhite = false;
          {
            std::unique_lock<std::mutex> Locker(*WhiteMutex);
            IsWhite = White->erase(H) > 0;
          }
          if (IsWhite) {
            std::unique_lock<std::mutex> Locker(GrayMutex);
            Gray.push_back(H);
          }
        }
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
              continue;
            }
            // All other workers are waiting, start sweeping
            {
              for (auto *H : *White) {
                do_deallocate(reinterpret_cast<uint8_t *>(H), H->Size);
              }
              White->clear();
              std::swap(BlackMutex, WhiteMutex);
              std::swap(Black, White);
            }
            {
              // mark root
              std::unique_lock<std::mutex> Locker(RootMutex);
              for (auto *H : Root) {
                if (H->IsGray.exchange(true, std::memory_order_acq_rel) ==
                    false) {
                  Gray.push_back(H);
                }
              }
            }
            const auto Duration = NextGC.fetch_add(
                std::chrono::steady_clock::duration(std::chrono::seconds(1))
                    .count(),
                std::memory_order_relaxed);
            std::this_thread::sleep_until(std::chrono::steady_clock::time_point(
                std::chrono::steady_clock::duration(Duration)));
            WorkerCount.fetch_add(1, std::memory_order_acq_rel);
            GrayNotEmptyCV.notify_all();
            continue;
          } else {
            H = Gray.front();
            Gray.pop_front();
          }
        }
        // Mark all children of object H as gray
        uint8_t *const Pointers =
            reinterpret_cast<uint8_t *>(H) + sizeof(Header);
        const auto &Type = *H->Type;
        assuming(H->Size == Type.size() - 1);
        switch (Type[0]) {
        case TypeCode::Array:
          if (IsRef(Type[1])) {
            for (size_t I = 0; I < H->Size; ++I) {
              MarkGray(Pointers + I);
            }
          }
          break;
        case TypeCode::Struct:
          for (size_t I = 0; I < H->Size; ++I) {
            if (IsRef(Type[I + 1])) {
              MarkGray(Pointers + I);
            }
          }
          break;
        default:
          assumingUnreachable();
        }
        {
          std::unique_lock<std::mutex> Locker(*BlackMutex);
          Black->emplace(H);
        }
        H->IsGray.store(false, std::memory_order_release);
        WorkerCount.fetch_sub(1, std::memory_order_acq_rel);
        break;
      }
    });
  }
}

Allocator::~Allocator() noexcept {
  Stop.store(true, std::memory_order_release);
  for (auto &Collector : Collectors) {
    Collector.join();
  }
}

[[nodiscard]] uint8_t *Allocator::do_allocate(uint32_t N) noexcept {
  if (Used.fetch_add(N, std::memory_order_acquire) >
      Threshold.load(std::memory_order_acquire)) {
    return nullptr;
  }
  return static_cast<uint8_t *>(std::malloc(N));
}

void Allocator::do_deallocate(uint8_t *P, uint32_t N) noexcept {
  Used.fetch_sub(N, std::memory_order_release);
  std::free(P);
}

void Allocator::do_collect() noexcept {
  std::chrono::steady_clock::time_point Start =
      std::chrono::steady_clock::now();
  uint64_t Freed = 0;

  // TODO: Implement the garbage collection algorithm here.

  std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
  spdlog::info(
      "GC: Freed {} bytes in {} ms"sv, Freed,
      std::chrono::duration_cast<std::chrono::milliseconds>(End - Start)
          .count());
}

} // namespace WasmEdge::GC
