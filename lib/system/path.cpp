// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "system/path.h"

#include "common/config.h"
#include "common/defines.h"
#include <string_view>

#if defined(HAVE_PWD_H)
#include <pwd.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include "common/errcode.h"
#include "system/winapi.h"
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
#if NTDDI_VERSION >= NTDDI_VISTA
    wchar_t *Path = nullptr;
    if (winapi::HRESULT_ Res = winapi::SHGetKnownFolderPath(
            winapi::FOLDERID_LocalAppData, winapi::KF_FLAG_CREATE_, nullptr,
            &Path);
        winapi::SUCCEEDED_(Res)) {
      Home = std::filesystem::path(Path);
      winapi::CoTaskMemFree(Path);
    }
#else
    wchar_t Path[winapi::MAX_PATH_];
    if (winapi::HRESULT_ Res = winapi::SHGetFolderPathW(
            nullptr, winapi::CSIDL_LOCAL_APPDATA_ | winapi::CSIDL_FLAG_CREATE_,
            nullptr, 0, Path);
        winapi::SUCCEEDED_(Res)) {
      Home = std::filesystem::path(Path);
    }
#endif
  }
#endif
  if (!Home.empty()) {
    return Home / std::filesystem::u8path(".wasmedge"sv);
  }
  return {};
}

} // namespace WasmEdge
