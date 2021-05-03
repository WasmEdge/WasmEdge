// SPDX-License-Identifier: Apache-2.0
#include "loader/shared_library.h"
#include "common/defines.h"
#include "common/log.h"

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
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
  Handle = boost::winapi::load_library_ex(Path.c_str(), 0, 0);
#else
  Handle = ::dlopen(Path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
  if (!Handle) {
    return Unexpect(ErrCode::InvalidPath);
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
  return boost::winapi::get_proc_address(Handle, Name);
#else
  return ::dlsym(Handle, Name);
#endif
}

} // namespace Loader
} // namespace WasmEdge
