// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/shared_library.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <utility>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#else
#error Unsupported os!
#endif

namespace WasmEdge::Loader {

// Open so file. See "include/loader/shared_library.h".
Expect<void> SharedLibrary::load(const std::filesystem::path &Path) noexcept {
#if WASMEDGE_OS_WINDOWS
  Handle = winapi::LoadLibraryExW(Path.c_str(), nullptr, 0);
#else
  Handle = ::dlopen(Path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
  if (!Handle) {
    spdlog::error(ErrCode::Value::IllegalPath);
#if WASMEDGE_OS_WINDOWS
    const auto Code = winapi::GetLastError();
    winapi::LPSTR_ ErrorText = nullptr;
    if (winapi::FormatMessageA(winapi::FORMAT_MESSAGE_FROM_SYSTEM_ |
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

void SharedLibrary::unload() noexcept {
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
  return reinterpret_cast<void *>(winapi::GetProcAddress(Handle, Name));
#else
  return ::dlsym(Handle, Name);
#endif
}

} // namespace WasmEdge::Loader
