// SPDX-License-Identifier: Apache-2.0
#include "system/allocator.h"
#include "common/defines.h"
#include "common/errcode.h"
#include "config.h"
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#ifdef HAVE_MMAP
#define ALLOC_MMAP 1
#else
#define ALLOC_STUB 1
#endif
#elif WASMEDGE_OS_WINDOWS
#define ALLOC_WINAPI 1
#endif

#if ALLOC_MMAP
#include <sys/mman.h>
#elif MAP_WINAPI
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/page_protection_flags.hpp>
#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {
BOOST_SYMBOL_IMPORT boost::winapi::LPVOID_ BOOST_WINAPI_WINAPI_CC VirtualAlloc(
    boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize,
    boost::winapi::DWORD_ flAllocationType, boost::winapi::DWORD_ flProtect);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
VirtualFree(boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize,
            boost::winapi::DWORD_ dwFreeType);
}
#endif
namespace boost {
namespace winapi {
using ::VirtualAlloc;
using ::VirtualFree;
#if defined(BOOST_USE_WINDOWS_H)
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_COMMIT_ = MEM_COMMIT;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RESERVE_ = MEM_RESERVE;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_DECOMMIT_ = MEM_DECOMMIT;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RELEASE_ = MEM_RELEASE;
#else
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_COMMIT_ = 0x00001000;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RESERVE_ = 0x00002000;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_DECOMMIT_ = 0x00004000;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RELEASE_ = 0x00008000;
#endif
} // namespace winapi
} // namespace boost
#endif

namespace WasmEdge {

namespace {
static inline constexpr const uint64_t kPageSize = UINT64_C(65536);
static inline constexpr const uint64_t kPageMask = UINT64_C(65535);
static inline constexpr const uint64_t k4G = UINT64_C(0x100000000);
static inline constexpr const uint64_t k8G = UINT64_C(0x200000000);
static inline constexpr const uint64_t k12G = k4G + k8G;

#if ALLOC_MMAP
static std::mutex Mutex;
static std::set<std::pair<uintptr_t, uintptr_t>> Regions;
#endif
} // namespace

uint8_t *Allocator::allocate(uint32_t PageCount) noexcept {
#if ALLOC_MMAP
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
#elif MAP_WINAPI
  auto Reserved = reinterpret_cast<uint8_t *>(
      boost::winapi::VirtualAlloc(nullptr, k12G, boost::winapi::MEM_RESERVE_,
                                  boost::winapi::PAGE_NOACCESS_));
  if (Reserved == nullptr) {
    return nullptr;
  }
  if (PageCount == 0) {
    return Reserved + k4G;
  }
  auto Pointer = resize(Reserved + k4G, 0, PageCount);
  if (Pointer == nullptr) {
    return nullptr;
  }
  return Pointer;
#elif MAP_STUB
  return std::malloc(kPageSize);
#endif
}

uint8_t *Allocator::resize(uint8_t *Pointer, uint32_t OldPageCount,
                           uint32_t NewPageCount) noexcept {
  assert(NewPageCount > OldPageCount);
#if ALLOC_MMAP
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
#elif MAP_WINAPI
  if (boost::winapi::VirtualAlloc(Pointer + OldPageCount * kPageSize,
                                  (NewPageCount - OldPageCount) * kPageSize,
                                  boost::winapi::MEM_COMMIT_,
                                  boost::winapi::PAGE_READWRITE_) == nullptr) {
    return nullptr;
  }
  return Pointer;
#elif MAP_STUB
  return std::realloc(Pointer, PageCount * kPageSize);
#endif
}

void Allocator::release(uint8_t *Pointer, uint32_t PageCount) noexcept {
#if ALLOC_MMAP
  if (Pointer == nullptr) {
    return;
  }
  std::unique_lock Lock(Mutex);
  const auto PaddedBegin = reinterpret_cast<uintptr_t>(Pointer) - k4G;
  Regions.erase(std::pair(PaddedBegin, PaddedBegin + k12G));
  munmap(Pointer, PageCount * kPageSize);
#elif MAP_WINAPI
  boost::winapi::VirtualFree(Pointer - k4G, 0, boost::winapi::MEM_RELEASE_);
#elif MAP_STUB
  return std::free(Pointer);
#endif
}

} // namespace WasmEdge
