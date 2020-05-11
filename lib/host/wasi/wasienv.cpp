// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasienv.h"

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

WasiEnvironment::WasiEnvironment() {
  using namespace std::string_view_literals;
  PreStats.emplace_back(STDIN_FILENO, kStdInRights, 0, "/dev/stdin"sv);
  PreStats.emplace_back(STDOUT_FILENO, kStdOutRights, 0, "/dev/stdout"sv);
  PreStats.emplace_back(STDERR_FILENO, kStdErrRights, 0, "/dev/stderr"sv);
  /// Open dir for WASI environment.
  PreStats.emplace_back(open(".", O_RDONLY | O_DIRECTORY), kDirectoryRights,
                        kInheritingDirectoryRights, "."sv);
}

WasiEnvironment::~WasiEnvironment() noexcept {
  for (const auto &Entry : PreStats) {
    close(Entry.Fd);
  }
}

} // namespace Host
} // namespace SSVM
