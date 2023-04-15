// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/shared_library.h"

#include "common/log.h"
#include "system/allocator.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <utility>

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
#include <boost/winapi/error_handling.hpp>
#include <boost/winapi/local_memory.hpp>

#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {
struct _IMAGE_RUNTIME_FUNCTION_ENTRY;
}
#endif

namespace boost::winapi {
typedef struct BOOST_MAY_ALIAS _IMAGE_RUNTIME_FUNCTION_ENTRY {
  DWORD_ BeginAddress;
  DWORD_ EndAddress;
  union {
    DWORD_ UnwindInfoAddress;
    DWORD_ UnwindData;
  } DUMMYUNIONNAME;
} RUNTIME_FUNCTION_, *PRUNTIME_FUNCTION_;
} // namespace boost::winapi

#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {
BOOST_SYMBOL_IMPORT boost::winapi::BOOLEAN_ BOOST_WINAPI_WINAPI_CC
RtlAddFunctionTable(boost::winapi::PRUNTIME_FUNCTION_ FunctionTable,
                    boost::winapi::ULONG_ EntryCount,
                    boost::winapi::ULONG_PTR_ BaseAddress);
BOOST_SYMBOL_IMPORT boost::winapi::BOOLEAN_ BOOST_WINAPI_WINAPI_CC
RtlDeleteFunctionTable(boost::winapi::PRUNTIME_FUNCTION_ FunctionTable);
}
#endif
namespace boost::winapi {
using ::RtlAddFunctionTable;
using ::RtlDeleteFunctionTable;
} // namespace boost::winapi

namespace winapi = boost::winapi;
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#else
#error Unsupported os!
#endif

namespace {
inline constexpr uint64_t roundDownPageBoundary(const uint64_t Value) {
// ARM64 Mac has a special page size
#if WASMEDGE_OS_MACOS && defined(__aarch64__)
  return Value & ~UINT64_C(16383);
#else
  return Value & ~UINT64_C(4095);
#endif
}
inline constexpr uint64_t roundUpPageBoundary(const uint64_t Value) {
// ARM64 Mac has a special page size
#if WASMEDGE_OS_MACOS && defined(__aarch64__)
  return roundDownPageBoundary(Value + UINT64_C(16383));
#else
  return roundDownPageBoundary(Value + UINT64_C(4095));
#endif
}
} // namespace

namespace WasmEdge {
namespace Loader {

// Open so file. See "include/loader/shared_library.h".
Expect<void> SharedLibrary::load(const std::filesystem::path &Path) noexcept {
#if WASMEDGE_OS_WINDOWS
  Handle = winapi::load_library_ex(Path.c_str(), nullptr, 0);
#else
  Handle = ::dlopen(Path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
  if (!Handle) {
    spdlog::error(ErrCode::Value::IllegalPath);
#if WASMEDGE_OS_WINDOWS
    const auto Code = winapi::GetLastError();
    winapi::LPSTR_ ErrorText = nullptr;
    if (winapi::format_message(winapi::FORMAT_MESSAGE_FROM_SYSTEM_ |
                                   winapi::FORMAT_MESSAGE_ALLOCATE_BUFFER_ |
                                   winapi::FORMAT_MESSAGE_IGNORE_INSERTS_,
                               nullptr, Code,
                               winapi::MAKELANGID_(winapi::LANG_NEUTRAL_,
                                                   winapi::SUBLANG_DEFAULT_),
                               reinterpret_cast<winapi::LPSTR_>(&ErrorText), 0,
                               nullptr)) {
      spdlog::error("    load library failed:{}", ErrorText);
      winapi::LocalFree(ErrorText);
    } else {
      spdlog::error("    load library failed:{:x}", Code);
    }
#else
    spdlog::error("    load library failed:{}", ::dlerror());
#endif
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  return {};
}

Expect<void> SharedLibrary::load(const AST::AOTSection &AOTSec) noexcept {
  BinarySize = 0;
  for (const auto &Section : AOTSec.getSections()) {
    const auto Offset = std::get<1>(Section);
    const auto Size = std::get<2>(Section);
    BinarySize = std::max(BinarySize, Offset + Size);
  }
  BinarySize = roundUpPageBoundary(BinarySize);

  Binary = Allocator::allocate_chunk(BinarySize);
  if (unlikely(!Binary)) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::vector<std::pair<uint8_t *, uint64_t>> ExecutableRanges;
  for (const auto &Section : AOTSec.getSections()) {
    const auto Offset = std::get<1>(Section);
    const auto Size = std::get<2>(Section);
    const auto &Content = std::get<3>(Section);
    assuming(Content.size() <= Size);
    std::copy(Content.begin(), Content.end(), Binary + Offset);
    switch (std::get<0>(Section)) {
    case 1: { // Text
      const auto O = roundDownPageBoundary(Offset);
      const auto S = roundUpPageBoundary(Size + (Offset - O));
      ExecutableRanges.emplace_back(Binary + O, S);
      break;
    }
    case 2: // Data
      break;
    case 3: // BSS
      break;
#if WASMEDGE_OS_WINDOWS
    case 4: // PData
      PDataAddress = reinterpret_cast<void *>(Binary + Offset);
      PDataSize =
          static_cast<uint32_t>(Size / sizeof(winapi::RUNTIME_FUNCTION_));
      break;
#endif
    }
  }

  for (const auto &[Pointer, Size] : ExecutableRanges) {
    if (!Allocator::set_chunk_executable(Pointer, Size)) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error("    set_chunk_executable failed:{}", std::strerror(errno));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
  }

  IntrinsicsAddress = AOTSec.getIntrinsicsAddress();
  TypesAddress = AOTSec.getTypesAddress();
  CodesAddress = AOTSec.getCodesAddress();

#if WASMEDGE_OS_WINDOWS
  if (PDataSize != 0) {
    winapi::RtlAddFunctionTable(
        static_cast<winapi::PRUNTIME_FUNCTION_>(PDataAddress), PDataSize,
        reinterpret_cast<winapi::ULONG_PTR_>(Binary));
  }
#endif

  return {};
}

void SharedLibrary::unload() noexcept {
  if (Binary) {
#if WASMEDGE_OS_WINDOWS
    if (PDataSize != 0) {
      winapi::RtlDeleteFunctionTable(
          static_cast<winapi::PRUNTIME_FUNCTION_>(PDataAddress));
    }
#endif
    Allocator::set_chunk_readable_writable(Binary, BinarySize);
    Allocator::release_chunk(Binary, BinarySize);
    Binary = nullptr;
  }
  if (Handle) {
#if WASMEDGE_OS_WINDOWS
    winapi::FreeLibrary(Handle);
#else
    ::dlclose(Handle);
#endif
    Handle = NativeHandle{};
  }
}

void *SharedLibrary::getSymbolAddr(const char *Name) const noexcept {
  if (!Handle) {
    return nullptr;
  }
#if WASMEDGE_OS_WINDOWS
  return reinterpret_cast<void *>(winapi::get_proc_address(Handle, Name));
#else
  return ::dlsym(Handle, Name);
#endif
}

uintptr_t SharedLibrary::getOffset() const noexcept {
  return reinterpret_cast<uintptr_t>(Binary);
}

} // namespace Loader
} // namespace WasmEdge
