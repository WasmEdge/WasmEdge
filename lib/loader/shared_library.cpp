// SPDX-License-Identifier: Apache-2.0
#include "loader/shared_library.h"
#include "common/defines.h"
#include "common/log.h"

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
#include <boost/winapi/error_handling.hpp>
#include <boost/winapi/local_memory.hpp>
namespace winapi = boost::winapi;
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#else
#error Unsupported os!
#endif

namespace WasmEdge {
namespace Loader {

/// Open so file. See "include/loader/shared_library.h".
Expect<void> SharedLibrary::load(const std::filesystem::path &Path) noexcept {
#if WASMEDGE_OS_WINDOWS
  Handle = winapi::load_library_ex(Path.c_str(), nullptr, 0);
#else
  Handle = ::dlopen(Path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
  if (!Handle) {
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
      spdlog::error("load library failed:{}", ErrorText);
      winapi::LocalFree(ErrorText);
    } else {
      spdlog::error("load library failed:{:x}", Code);
    }
#else
    spdlog::error("load library failed:{}", ::dlerror());
#endif
    return Unexpect(ErrCode::IllegalPath);
  }
  return {};
}

void SharedLibrary::unload() noexcept {
  if (Handle) {
#if WASMEDGE_OS_WINDOWS
    boost::winapi::FreeLibrary(Handle);
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
  return reinterpret_cast<void *>(
      boost::winapi::get_proc_address(Handle, Name));
#else
  return ::dlsym(Handle, Name);
#endif
}

} // namespace Loader
} // namespace WasmEdge
