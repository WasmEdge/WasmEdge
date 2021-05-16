// SPDX-License-Identifier: Apache-2.0
#include "system/allocator.h"
#include "common/errcode.h"
#include "config.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

namespace WasmEdge {

namespace {
static inline constexpr const uint64_t kPageSize = UINT64_C(65536);
static inline constexpr const uint64_t kPageMask = UINT64_C(65535);
static inline constexpr const uint64_t k4G = UINT64_C(0x100000000);
static inline constexpr const uint64_t k8G = UINT64_C(0x200000000);
static inline constexpr const uint64_t k12G = k4G + k8G;

#ifdef HAVE_MMAP
static std::mutex Mutex;
static std::set<std::pair<uintptr_t, uintptr_t>> Regions;
#endif
} // namespace

uint8_t *Allocator::allocate(uint32_t PageCount) noexcept {
#ifdef HAVE_MMAP
  /// Parse smaps for available memory region
  std::unique_lock Lock(Mutex);

  if (unlikely(Regions.empty())) {
    std::ifstream Smaps("/proc/self/smaps");
    for (std::string Line; std::getline(Smaps, Line);) {
      if (std::isdigit(Line.front()) || std::islower(Line.front())) {
        char *Cursor = nullptr;
        const uintptr_t Begin = std::strtoull(Line.c_str(), &Cursor, 16);
        const uintptr_t End = std::strtoull(++Cursor, &Cursor, 16);
        Regions.emplace(Begin, End);
      }
    }
  }

  for (auto Prev = Regions.cbegin(), Next = std::next(Prev);
       Next != Regions.cend(); ++Prev, ++Next) {
    if (Next->first - Prev->second >= k12G) {
      const auto PaddedBegin = (Prev->second + kPageMask) & ~kPageMask;
      if (Next->first - PaddedBegin >= k12G) {
        uint8_t *Pointer = reinterpret_cast<uint8_t *>(PaddedBegin + k4G);
        if (PageCount != 0 && resize(Pointer, 0, PageCount) == nullptr) {
          continue;
        }
        Regions.emplace(PaddedBegin, PaddedBegin + k12G);
        return Pointer;
      }
    }
  }
  return nullptr;
#else
  return std::malloc(kPageSize);
#endif
}

uint8_t *Allocator::resize(uint8_t *Pointer, uint32_t OldPageCount,
                           uint32_t NewPageCount) noexcept {
#ifdef HAVE_MMAP
  if (OldPageCount == 0) {
    if (mmap(Pointer, NewPageCount * kPageSize, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) {
      return nullptr;
    }
  } else {
    if (mremap(Pointer, OldPageCount * kPageSize, NewPageCount * kPageSize,
               0) == MAP_FAILED) {
      return nullptr;
    }
  }
  return Pointer;
#else
  return std::realloc(Pointer, PageCount * kPageSize);
#endif
}

void Allocator::release(uint8_t *Pointer, uint32_t PageCount) noexcept {
#ifdef HAVE_MMAP
  if (Pointer == nullptr) {
    return;
  }
  std::unique_lock Lock(Mutex);
  const auto PaddedBegin = reinterpret_cast<uintptr_t>(Pointer) - k4G;
  Regions.erase(std::pair(PaddedBegin, PaddedBegin + k12G));
  munmap(Pointer, PageCount * kPageSize);
#else
  return std::free(Pointer);
#endif
}

} // namespace WasmEdge
