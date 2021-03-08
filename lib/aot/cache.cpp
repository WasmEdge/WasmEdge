// SPDX-License-Identifier: Apache-2.0
#include "aot/cache.h"
#include "common/config.h"
#include "common/defines.h"
#include "common/log.h"

#if SSVM_OS_LINUX || SSVM_OS_MACOS
#include <pwd.h>
#include <unistd.h>
#endif

namespace SSVM {
namespace AOT {

namespace {
std::filesystem::path getRoot(Cache::StorageScope Scope) {
  switch (Scope) {
  case Cache::StorageScope::Global:
    return std::filesystem::u8path(kCacheRoot);
  case Cache::StorageScope::Local: {
#if SSVM_OS_LINUX || SSVM_OS_MACOS
    const struct passwd *PassWd = getpwuid(getuid());
    return std::filesystem::u8path(PassWd->pw_dir) / ".ssvm/cache"sv;
#else
    return {};
#endif
  }
  default:
    __builtin_unreachable();
  }
}
} // namespace

Expect<std::filesystem::path> Cache::getPath(Span<const Byte> Data,
                                             Cache::StorageScope Scope,
                                             std::string_view Key) {
  auto Root = getRoot(Scope);
  if (!Key.empty()) {
    Root /= std::filesystem::u8path(Key);
  }

  return Root / std::filesystem::u8path(Key);
}

} // namespace AOT
} // namespace SSVM
