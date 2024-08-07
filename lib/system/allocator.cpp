// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "system/allocator.h"

#include "common/config.h"
#include "common/defines.h"
#include "common/errcode.h"

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__) ||     \
    defined(__arm__) || (defined(__riscv) && __riscv_xlen == 64)
#include <sys/mman.h>
#else
#include <cctype>
#include <cstdlib>
#include <cstring>
#endif

namespace WasmEdge {

namespace {
static inline constexpr const uint64_t kPageSize = UINT64_C(65536);

#if WASMEDGE_OS_WINDOWS || defined(HAVE_MMAP) && defined(__x86_64__) ||        \
    defined(__aarch64__) || (defined(__riscv) && __riscv_xlen == 64)
// Only define these two constants on the supported platform to avoid
// -Wunused-const-variable error when applying -Werror.
static inline constexpr const uint64_t k4G = UINT64_C(0x100000000);
static inline constexpr const uint64_t k12G = UINT64_C(0x300000000);
#endif

} // namespace

WASMEDGE_EXPORT uint8_t *Allocator::allocate(uint32_t PageCount) noexcept {
#if WASMEDGE_OS_WINDOWS
  auto Reserved = reinterpret_cast<uint8_t *>(winapi::VirtualAlloc(
      nullptr, k12G, winapi::MEM_RESERVE_, winapi::PAGE_NOACCESS_));
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
#elif defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__) ||     \
    (defined(__riscv) && __riscv_xlen == 64)
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
#else
  auto Result = reinterpret_cast<uint8_t *>(std::malloc(kPageSize * PageCount));
  if (Result == nullptr) {
    return nullptr;
  }
  std::memset(Result, 0, kPageSize * PageCount);
  return Result;
#endif
}

WASMEDGE_EXPORT uint8_t *Allocator::resize(uint8_t *Pointer,
                                           uint32_t OldPageCount,
                                           uint32_t NewPageCount) noexcept {
  assuming(NewPageCount > OldPageCount);
#if WASMEDGE_OS_WINDOWS
  if (winapi::VirtualAlloc(Pointer + OldPageCount * kPageSize,
                           (NewPageCount - OldPageCount) * kPageSize,
                           winapi::MEM_COMMIT_,
                           winapi::PAGE_READWRITE_) == nullptr) {
    return nullptr;
  }
  return Pointer;
#elif defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__) ||     \
    (defined(__riscv) && __riscv_xlen == 64)
  if (mmap(Pointer + OldPageCount * kPageSize,
           (NewPageCount - OldPageCount) * kPageSize, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) {
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

WASMEDGE_EXPORT void Allocator::release(uint8_t *Pointer, uint32_t) noexcept {
#if WASMEDGE_OS_WINDOWS
  winapi::VirtualFree(Pointer - k4G, 0, winapi::MEM_RELEASE_);
#elif defined(HAVE_MMAP) && defined(__x86_64__) || defined(__aarch64__) ||     \
    (defined(__riscv) && __riscv_xlen == 64)
  if (Pointer == nullptr) {
    return;
  }
  munmap(Pointer - k4G, k12G);
#else
  return std::free(Pointer);
#endif
}

uint8_t *Allocator::allocate_chunk(uint64_t Size) noexcept {
#if WASMEDGE_OS_WINDOWS
  if (auto Pointer = winapi::VirtualAlloc(nullptr, Size, winapi::MEM_COMMIT_,
                                          winapi::PAGE_READWRITE_);
      unlikely(Pointer == nullptr)) {
    return nullptr;
  } else {
    return reinterpret_cast<uint8_t *>(Pointer);
  }
#elif defined(HAVE_MMAP)
  if (auto Pointer = mmap(nullptr, Size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      unlikely(Pointer == MAP_FAILED)) {
    return nullptr;
  } else {
    return reinterpret_cast<uint8_t *>(Pointer);
  }
#else
  return std::malloc(Size);
#endif
}

void Allocator::release_chunk(uint8_t *Pointer,
                              uint64_t Size [[maybe_unused]]) noexcept {
#if WASMEDGE_OS_WINDOWS
  winapi::VirtualFree(Pointer, 0, winapi::MEM_RELEASE_);
#elif defined(HAVE_MMAP)
  munmap(Pointer, Size);
#else
  return std::free(Pointer);
#endif
}

bool Allocator::set_chunk_executable(uint8_t *Pointer, uint64_t Size) noexcept {
#if WASMEDGE_OS_WINDOWS
  winapi::DWORD_ OldPerm;
  return winapi::VirtualProtect(Pointer, Size, winapi::PAGE_EXECUTE_READ_,
                                &OldPerm) != 0;
#elif defined(HAVE_MMAP)
  return mprotect(Pointer, Size, PROT_EXEC | PROT_READ) == 0;
#else
  return true;
#endif
}

bool Allocator::set_chunk_readable(uint8_t *Pointer, uint64_t Size) noexcept {
#if WASMEDGE_OS_WINDOWS
  winapi::DWORD_ OldPerm;
  return winapi::VirtualProtect(Pointer, Size, winapi::PAGE_READONLY_,
                                &OldPerm) != 0;
#elif defined(HAVE_MMAP)
  return mprotect(Pointer, Size, PROT_READ) == 0;
#else
  return true;
#endif
}

bool Allocator::set_chunk_readable_writable(uint8_t *Pointer,
                                            uint64_t Size) noexcept {
#if WASMEDGE_OS_WINDOWS
  winapi::DWORD_ OldPerm;
  return winapi::VirtualProtect(Pointer, Size, winapi::PAGE_READWRITE_,
                                &OldPerm) != 0;
#elif defined(HAVE_MMAP)
  return mprotect(Pointer, Size, PROT_READ | PROT_WRITE) == 0;
#else
  return true;
#endif
}

} // namespace WasmEdge
