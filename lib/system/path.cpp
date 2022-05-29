// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "system/path.h"

#include "common/config.h"
#include "common/defines.h"
#include <string_view>

#if defined(HAVE_PWD_H)
#include <pwd.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include "common/errcode.h"
#include <shlobj_core.h>
#endif

namespace WasmEdge {

std::filesystem::path Path::home() noexcept {
  using namespace std::literals::string_view_literals;
  std::filesystem::path Home;
#if defined(HAVE_PWD_H)
  {
    const struct passwd *PassWd = getpwuid(getuid());
    Home = std::filesystem::u8path(PassWd->pw_dir);
  }
#elif WASMEDGE_OS_WINDOWS
  {
    wchar_t *Path;
    if (auto Res = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE,
                                          nullptr, &Path);
        likely(Res == S_OK)) {
      Home = Path;
      ::CoTaskMemFree(Path);
    }
  }
#endif
  if (!Home.empty()) {
    return Home / std::filesystem::u8path(".wasmedge"sv);
  }
  return {};
}

} // namespace WasmEdge
