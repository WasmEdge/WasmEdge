// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "host/wasi/environ.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "host/wasi/vfs.h"
#include "host/wasi/vinode.h"

using namespace std::literals;

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {
static inline constexpr const __wasi_rights_t kPreOpenBaseRightsReadOnly =
    __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READDIR |
    __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_FILESTAT_GET |
    __WASI_RIGHTS_FD_FILESTAT_GET;
static inline constexpr const __wasi_rights_t kPreOpenInheritingRightsReadOnly =
    __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK |
    __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS | __WASI_RIGHTS_FD_SYNC |
    __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_FD_ADVISE |
    __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
    __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_LINK_TARGET |
    __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READDIR |
    __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_RENAME_SOURCE |
    __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_FILESTAT_GET |
    __WASI_RIGHTS_FD_FILESTAT_GET | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHTS_PATH_SYMLINK | __WASI_RIGHTS_PATH_REMOVE_DIRECTORY |
    __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kPreOpenBaseRights =
    __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
    __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_LINK_TARGET |
    __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READDIR |
    __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_RENAME_SOURCE |
    __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_FILESTAT_GET |
    __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_FILESTAT_SET_TIMES | __WASI_RIGHTS_PATH_SYMLINK |
    __WASI_RIGHTS_PATH_REMOVE_DIRECTORY | __WASI_RIGHTS_PATH_UNLINK_FILE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE;
static inline constexpr const __wasi_rights_t kPreOpenInheritingRights =
    __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK |
    __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS | __WASI_RIGHTS_FD_SYNC |
    __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_ADVISE |
    __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_PATH_CREATE_DIRECTORY |
    __WASI_RIGHTS_PATH_CREATE_FILE | __WASI_RIGHTS_PATH_LINK_SOURCE |
    __WASI_RIGHTS_PATH_LINK_TARGET | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_PATH_READLINK |
    __WASI_RIGHTS_PATH_RENAME_SOURCE | __WASI_RIGHTS_PATH_RENAME_TARGET |
    __WASI_RIGHTS_PATH_FILESTAT_GET | __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHTS_PATH_SYMLINK | __WASI_RIGHTS_PATH_REMOVE_DIRECTORY |
    __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE |
    __WASI_RIGHTS_SOCK_SHUTDOWN;
static inline constexpr const __wasi_rights_t kStdInDefaultRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdOutDefaultRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_DATASYNC |
    __WASI_RIGHTS_FD_FILESTAT_GET | __WASI_RIGHTS_FD_SYNC |
    __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdErrDefaultRights =
    kStdOutDefaultRights;
static inline constexpr const __wasi_rights_t kNoInheritingRights =
    static_cast<__wasi_rights_t>(0);
static inline constexpr const auto kReadOnly = "readonly"sv;

} // namespace

void Environ::init(Span<const std::string> Dirs, const std::string &ProgramName,
                   Span<const std::string> Args, Span<const std::string> Envs) {
  {
    // Open dir for WASI environment.
    std::vector<std::shared_ptr<VINode>> PreopenedDirs;
    PreopenedDirs.reserve(Dirs.size());
    for (const auto &Dir : Dirs) {
      const auto Pos = Dir.find(':');
      std::string HostDir =
          (Pos == std::string::npos) ? Dir : Dir.substr(Pos + 1);
      // Handle the readonly flag
      bool ReadOnly = false;
      if (const auto ROPos = HostDir.find(':'); ROPos != std::string::npos) {
        const auto Mode = HostDir.substr(ROPos + 1);
        HostDir = HostDir.substr(0, ROPos);
        if (kReadOnly == Mode) {
          ReadOnly = true;
        }
      }
      std::string GuestDir = VINode::canonicalGuest(
          (Pos == std::string::npos) ? std::string_view(Dir)
                                     : std::string_view(Dir).substr(0, Pos));
      if (GuestDir.size() == 0) {
        GuestDir = '/';
      }
      const auto BaseRights =
          ReadOnly ? kPreOpenBaseRightsReadOnly : kPreOpenBaseRights;
      const auto InheritingRights = ReadOnly ? kPreOpenInheritingRightsReadOnly
                                             : kPreOpenInheritingRights;
      if (auto Res = VINode::bind(BaseRights, InheritingRights,
                                  std::move(GuestDir), std::move(HostDir));
          unlikely(!Res)) {
        spdlog::error("Bind guest directory failed:{}"sv, Res.error());
        continue;
      } else {
        PreopenedDirs.emplace_back(std::move(*Res));
      }
    }

    std::sort(PreopenedDirs.begin(), PreopenedDirs.end());

    FdMap.emplace(0, VINode::stdIn(kStdInDefaultRights, kNoInheritingRights));
    FdMap.emplace(1, VINode::stdOut(kStdOutDefaultRights, kNoInheritingRights));
    FdMap.emplace(2, VINode::stdErr(kStdErrDefaultRights, kNoInheritingRights));

    int NewFd = 3;
    for (auto &PreopenedDir : PreopenedDirs) {
      FdMap.emplace(NewFd++, std::move(PreopenedDir));
    }
  }

  Arguments.resize(Args.size() + 1);
  Arguments.front() = ProgramName;
  std::copy(Args.begin(), Args.end(), Arguments.begin() + 1);
  Arguments.shrink_to_fit();

  EnvironVariables.resize(Envs.size());
  std::copy(Envs.begin(), Envs.end(), EnvironVariables.begin());
  EnvironVariables.shrink_to_fit();

  ExitCode = 0;
}

void Environ::fini() noexcept {
  EnvironVariables.clear();
  Arguments.clear();
  FdMap.clear();
}

Environ::~Environ() noexcept { fini(); }

WasiExpect<bool> Environ::pathExists(std::string_view Path) const noexcept {
  __wasi_filestat_t Filestat;
  auto StatResult = const_cast<Environ *>(this)->pathFilestatGet(
      3, Path, static_cast<__wasi_lookupflags_t>(0), Filestat);
  if (StatResult) {
    return true;
  }
  if (StatResult.error() == __WASI_ERRNO_NOENT) {
    return false;
  }
  return WasiUnexpect(StatResult.error());
}

WasiExpect<bool> Environ::pathCanRead(std::string_view Path) const noexcept {
  __wasi_fd_t BaseFd = 3;
  auto Result = const_cast<Environ *>(this)->pathOpen(
      BaseFd, Path, static_cast<__wasi_lookupflags_t>(0),
      static_cast<__wasi_oflags_t>(0), __WASI_RIGHTS_FD_READ,
      __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));

  if (Result) {
    const_cast<Environ *>(this)->fdClose(*Result);
    return true;
  }
  if (Result.error() == __WASI_ERRNO_ACCES ||
      Result.error() == __WASI_ERRNO_NOENT) {
    return false;
  }
  return WasiUnexpect(Result.error());
}

WasiExpect<bool> Environ::pathCanWrite(std::string_view Path) const noexcept {
  __wasi_fd_t BaseFd = 3;
  auto Result = const_cast<Environ *>(this)->pathOpen(
      BaseFd, Path, static_cast<__wasi_lookupflags_t>(0),
      static_cast<__wasi_oflags_t>(0), __WASI_RIGHTS_FD_WRITE,
      __WASI_RIGHTS_FD_WRITE, static_cast<__wasi_fdflags_t>(0));

  if (Result) {
    const_cast<Environ *>(this)->fdClose(*Result);
    return true;
  }
  if (Result.error() == __WASI_ERRNO_ACCES ||
      Result.error() == __WASI_ERRNO_NOENT) {
    return false;
  }
  return WasiUnexpect(Result.error());
}

WasiExpect<__wasi_rights_t>
Environ::pathGetStats(std::string_view Path) const noexcept {
  __wasi_rights_t Rights = static_cast<__wasi_rights_t>(0);
  auto StatResult = pathExists(Path);
  if (StatResult.error()) {
    return WasiUnexpect(StatResult.error());
  }
  if (StatResult) {
    // File exists, so we can get file stats
    Rights |= __WASI_RIGHTS_PATH_FILESTAT_GET;
  }
  StatResult = pathCanRead(Path);
  if (StatResult.error()) {
    return WasiUnexpect(StatResult.error());
  }
  if (StatResult) {
    Rights |= __WASI_RIGHTS_FD_READ;
  }
  StatResult = pathCanWrite(Path);
  if (StatResult.error()) {
    return WasiUnexpect(StatResult.error());
  }
  if (StatResult) {
    Rights |= __WASI_RIGHTS_FD_WRITE;
  }
  return Rights;
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
