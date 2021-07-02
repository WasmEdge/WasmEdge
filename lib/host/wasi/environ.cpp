// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/environ.h"
#include "common/errcode.h"
#include "common/log.h"
#include "host/wasi/vfs.h"
#include "host/wasi/vinode.h"
#include <random>

using namespace std::literals;

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {
static inline constexpr const __wasi_rights_t kReadRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_FD_SEEK |
    __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_PATH_FILESTAT_GET |
    __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_RENAME_SOURCE |
    __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_SOCK_SHUTDOWN;
static inline constexpr const __wasi_rights_t kWriteRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_ALLOCATE |
    __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
    __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHTS_FD_SYNC | __WASI_RIGHTS_FD_WRITE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_PATH_REMOVE_DIRECTORY | __WASI_RIGHTS_PATH_RENAME_TARGET |
    __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE |
    __WASI_RIGHTS_SOCK_SHUTDOWN;
static inline constexpr const __wasi_rights_t kCreateRights =
    __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
    __WASI_RIGHTS_PATH_LINK_TARGET | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_SYMLINK;
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

} // namespace

void Environ::init(Span<const std::string> Dirs, std::string ProgramName,
                   Span<const std::string> Args, Span<const std::string> Envs) {
  {
    /// Open dir for WASI environment.
    std::vector<std::shared_ptr<VINode>> PreopenedDirs;
    PreopenedDirs.reserve(Dirs.size());
    for (const auto &Dir : Dirs) {
      const auto Pos = Dir.find(':');
      if (Pos != std::string::npos) {
        const auto HostDir = Dir.substr(Pos + 1);
        auto GuestDir =
            VINode::canonicalGuest(std::string_view(Dir).substr(0, Pos));
        if (GuestDir.size() == 0) {
          GuestDir = '/';
        }
        if (auto Res =
                VINode::bind(FS, kReadRights | kWriteRights | kCreateRights,
                             kReadRights | kWriteRights | kCreateRights,
                             std::move(GuestDir), std::move(HostDir));
            unlikely(!Res)) {
          spdlog::error("Bind guest directory failed:{}", Res.error());
          continue;
        } else {
          PreopenedDirs.emplace_back(std::move(*Res));
        }
      }
    }

    std::sort(PreopenedDirs.begin(), PreopenedDirs.end());

    FdMap.emplace(0,
                  VINode::stdIn(FS, kStdInDefaultRights, kNoInheritingRights));
    FdMap.emplace(
        1, VINode::stdOut(FS, kStdOutDefaultRights, kNoInheritingRights));
    FdMap.emplace(
        2, VINode::stdErr(FS, kStdErrDefaultRights, kNoInheritingRights));

    int NewFd = 3;
    for (auto &PreopenedDir : PreopenedDirs) {
      FdMap.emplace(NewFd++, std::move(PreopenedDir));
    }
  }

  Arguments.resize(Args.size() + 1);
  Arguments.front() = std::move(ProgramName);
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

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
