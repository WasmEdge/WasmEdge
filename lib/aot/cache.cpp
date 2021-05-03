// SPDX-License-Identifier: Apache-2.0
#include "aot/cache.h"
#include "aot/blake3.h"
#include "common/config.h"
#include "common/defines.h"
#include "common/hexstr.h"
#include "common/log.h"

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <pwd.h>
#include <unistd.h>
#endif

namespace WasmEdge {
namespace AOT {

namespace {
std::filesystem::path getRoot(Cache::StorageScope Scope) {
  switch (Scope) {
  case Cache::StorageScope::Global:
    return std::filesystem::u8path(kCacheRoot);
  case Cache::StorageScope::Local: {
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
    const struct passwd *PassWd = getpwuid(getuid());
    return std::filesystem::u8path(PassWd->pw_dir) / ".wasmedge/cache"sv;
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

  Blake3 Hasher;
  Hasher.update(Data);
  std::array<Byte, 32> Hash;
  Hasher.finalize(Hash);
  std::string HexStr;
  convertBytesToHexStr(Hash, HexStr);

  return Root / HexStr;
}

void Cache::clear(Cache::StorageScope Scope, std::string_view Key) {
  auto Root = getRoot(Scope);
  if (!Key.empty()) {
    Root /= std::filesystem::u8path(Key);
  }
  std::error_code ErrCode;
  std::filesystem::remove_all(Root, ErrCode);
}

} // namespace AOT
} // namespace WasmEdge
