// SPDX-License-Identifier: Apache-2.0
#include "system/path.h"
#include "config.h"

#if HAVE_PWD_H
#include <pwd.h>
#include <unistd.h>
#endif

namespace WasmEdge {

std::filesystem::path Path::home() noexcept {
#if HAVE_PWD_H
  using namespace std::literals::string_view_literals;
  const struct passwd *PassWd = getpwuid(getuid());
  return std::filesystem::u8path(PassWd->pw_dir) / ".wasmedge/cache"sv;
#else
  return {};
#endif
}

} // namespace WasmEdge
