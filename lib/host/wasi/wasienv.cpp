// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasienv.h"

extern char **environ;

namespace {
static inline constexpr const __wasi_rights_t kFileRights =
    __WASI_RIGHT_FD_DATASYNC | __WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_SEEK |
    __WASI_RIGHT_FD_FDSTAT_SET_FLAGS | __WASI_RIGHT_FD_SYNC |
    __WASI_RIGHT_FD_TELL | __WASI_RIGHT_FD_WRITE | __WASI_RIGHT_FD_ADVISE |
    __WASI_RIGHT_FD_ALLOCATE | __WASI_RIGHT_FD_FILESTAT_GET |
    __WASI_RIGHT_FD_FILESTAT_SET_SIZE | __WASI_RIGHT_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHT_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kDirectoryRights =
    __WASI_RIGHT_FD_FDSTAT_SET_FLAGS | __WASI_RIGHT_FD_SYNC |
    __WASI_RIGHT_FD_ADVISE | __WASI_RIGHT_PATH_CREATE_DIRECTORY |
    __WASI_RIGHT_PATH_CREATE_FILE | __WASI_RIGHT_PATH_LINK_SOURCE |
    __WASI_RIGHT_PATH_LINK_TARGET | __WASI_RIGHT_PATH_OPEN |
    __WASI_RIGHT_FD_READDIR | __WASI_RIGHT_PATH_READLINK |
    __WASI_RIGHT_PATH_RENAME_SOURCE | __WASI_RIGHT_PATH_RENAME_TARGET |
    __WASI_RIGHT_PATH_FILESTAT_GET | __WASI_RIGHT_PATH_FILESTAT_SET_SIZE |
    __WASI_RIGHT_PATH_FILESTAT_SET_TIMES | __WASI_RIGHT_FD_FILESTAT_GET |
    __WASI_RIGHT_FD_FILESTAT_SET_TIMES | __WASI_RIGHT_PATH_SYMLINK |
    __WASI_RIGHT_PATH_UNLINK_FILE | __WASI_RIGHT_PATH_REMOVE_DIRECTORY |
    __WASI_RIGHT_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kInheritingDirectoryRights =
    kFileRights | kDirectoryRights;
static inline constexpr const __wasi_rights_t kStdInRights =
    __WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
    __WASI_RIGHT_FD_FILESTAT_GET | __WASI_RIGHT_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdOutRights =
    __WASI_RIGHT_FD_WRITE | __WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
    __WASI_RIGHT_FD_FILESTAT_GET | __WASI_RIGHT_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdErrRights = kStdOutRights;
} // namespace

namespace SSVM {
namespace Host {

WasiEnvironment::WasiEnvironment() {}

WasiEnvironment::~WasiEnvironment() noexcept { fini(); }

void WasiEnvironment::fini() noexcept {
  for (const auto &File : FileMap) {
    if (File.second.HostFd != STDIN_FILENO &&
        File.second.HostFd != STDOUT_FILENO &&
        File.second.HostFd != STDERR_FILENO) {
      close(File.second.HostFd);
    }
  }
  FileMap.clear();
  Environs.clear();
  CmdArgs.clear();
}

void WasiEnvironment::init(Span<const std::string> Dirs,
                           std::string ProgramName,
                           Span<const std::string> Args,
                           Span<const std::string> Envs) {
  using namespace std::string_view_literals;

  emplaceFile(0, STDIN_FILENO, false, kStdInRights, 0, "/dev/stdin"sv);
  emplaceFile(1, STDOUT_FILENO, false, kStdOutRights, 0, "/dev/stdout"sv);
  emplaceFile(2, STDERR_FILENO, false, kStdErrRights, 0, "/dev/stderr"sv);

  /// Open dir for WASI environment.
  int NewFd = 3;
  for (const auto &Dir : Dirs) {
    const auto Pos = Dir.find(':');
    if (Pos != std::string::npos) {
      const auto GuestDir = Dir.substr(0, Pos);
      const auto HostDir = Dir.substr(Pos + 1);
      emplaceFile(NewFd++, open(HostDir.c_str(), O_PATH | O_DIRECTORY), true,
                  kDirectoryRights, kInheritingDirectoryRights, GuestDir);
    }
  }

  CmdArgs.resize(Args.size() + 1);
  CmdArgs.front() = std::move(ProgramName);
  std::copy(Args.begin(), Args.end(), CmdArgs.begin() + 1);
  CmdArgs.shrink_to_fit();

  Environs.resize(Envs.size());
  std::copy(Envs.begin(), Envs.end(), Environs.begin());
  Environs.shrink_to_fit();
}

} // namespace Host
} // namespace SSVM
