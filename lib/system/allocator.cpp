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

#if defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__)
#include <sys/mman.h>
#elif WASMEDGE_OS_WINDOWS
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
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RELEASE_ = MEM_RELEASE;
#else
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_COMMIT_ = 0x00001000;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RESERVE_ = 0x00002000;
BOOST_CONSTEXPR_OR_CONST DWORD_ MEM_RELEASE_ = 0x00008000;
#endif
} // namespace winapi
} // namespace boost
#endif

namespace WasmEdge {

namespace {
static inline constexpr const uint64_t kPageSize = UINT64_C(65536);
static inline constexpr const uint64_t k4G = UINT64_C(0x100000000);
static inline constexpr const uint64_t k12G = UINT64_C(0x300000000);

} // namespace

uint8_t *Allocator::allocate(uint32_t PageCount) noexcept {
#if defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__)
  auto Reserved = reinterpret_cast<uint8_t *>(
      mmap(nullptr, k12G, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0));
  if (Reserved == MAP_FAILED) {
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
#elif WASMEDGE_OS_WINDOWS
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
#else
  auto Result = reinterpret_cast<uint8_t *>(std::malloc(kPageSize * PageCount));
  if (Result == nullptr) {
    return nullptr;
  }
  std::memset(Result, 0, kPageSize * PageCount);
  return Result;
#endif
}

uint8_t *Allocator::resize(uint8_t *Pointer, uint32_t OldPageCount,
                           uint32_t NewPageCount) noexcept {
  assert(NewPageCount > OldPageCount);
#if defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__)
  if (mmap(Pointer + OldPageCount * kPageSize,
           (NewPageCount - OldPageCount) * kPageSize, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) {
    return nullptr;
  }
  return Pointer;
#elif WASMEDGE_OS_WINDOWS
  if (boost::winapi::VirtualAlloc(Pointer + OldPageCount * kPageSize,
                                  (NewPageCount - OldPageCount) * kPageSize,
                                  boost::winapi::MEM_COMMIT_,
                                  boost::winapi::PAGE_READWRITE_) == nullptr) {
    return nullptr;
  }
  return Pointer;
#else
  auto Result = reinterpret_cast<uint8_t *>(
      std::realloc(Pointer, NewPageCount * kPageSize));
  if (Result == nullptr) {
    return nullptr;
  }
  std::memset(Result + OldPageCount * kPageSize, 0,
              (NewPageCount - OldPageCount) * kPageSize);
  return Result;
#endif
}

void Allocator::release(uint8_t *Pointer, uint32_t) noexcept {
#if defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__)
  if (Pointer == nullptr) {
    return;
  }
  munmap(Pointer - k4G, k12G);
#elif WASMEDGE_OS_WINDOWS
  boost::winapi::VirtualFree(Pointer - k4G, 0, boost::winapi::MEM_RELEASE_);
#else
  return std::free(Pointer);
#endif
}

} // namespace WasmEdge
