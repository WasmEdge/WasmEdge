#include "shared_library.h"

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
#include <boost/winapi/error_handling.hpp>
namespace winapi = boost::winapi;
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#else
#error Unsupported os!
#endif

namespace WasmEdge {
namespace Host {
namespace WASINNTfLite {

SharedLibrary::SharedLibrary(const char *Name) noexcept {
#if WASMEDGE_OS_WINDOWS
  Handle = winapi::load_library_ex(Path.c_str(), nullptr, 0);
#else
  Handle = ::dlopen(Name, RTLD_LAZY | RTLD_LOCAL);
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
      spdlog::error("[WASI-NN] Load shared library failed: {}", ErrorText);
      winapi::LocalFree(ErrorText);
    } else {
      spdlog::error("[WASI-NN] Load shared library failed: {:x}", Code);
    }
#else
    spdlog::error("[WASI-NN] Load shared library failed: {}", ::dlerror());
#endif
  }

  spdlog::debug("[WASI-NN] Load shared library {} success.", Name);
}

void *SharedLibrary::getSymbolAddr(const char *Name) noexcept {
  if (Handle) {
#if WASMEDGE_OS_WINDOWS
    return winapi::get_proc_address(Handle, Name);
#else
    return ::dlsym(Handle, Name);
#endif
  }
  return nullptr;
}

SharedLibrary::~SharedLibrary() noexcept {
  if (Handle) {
#if WASMEDGE_OS_WINDOWS
    winapi::FreeLibrary(Handle);
#else
    ::dlclose(Handle);
#endif
  }
}

} // namespace WASINNTfLite
} // namespace Host
} // namespace WasmEdge
