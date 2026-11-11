// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
    __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_PATH_READLINK |
    __WASI_RIGHTS_PATH_FILESTAT_GET | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_POLL_FD_READWRITE;
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

WasiExpect<void>
Environ::initWithFds(Span<const std::string> Dirs, std::string ProgramName,
                     Span<const std::string> Args, Span<const std::string> Envs,
                     int32_t StdInFd, int32_t StdOutFd, int32_t StdErrFd) {
  {
    auto StdInVNode =
        VINode::fromFd(StdInFd, kStdInDefaultRights, kNoInheritingRights);
    if (!StdInVNode) {
      return WasiUnexpect(StdInVNode.error());
    }
    FdMap.emplace(0, std::move(*StdInVNode));
    auto StdOutVNode =
        VINode::fromFd(StdOutFd, kStdOutDefaultRights, kNoInheritingRights);
    if (!StdOutVNode) {
      return WasiUnexpect(StdOutVNode.error());
    }
    FdMap.emplace(1, std::move(*StdOutVNode));
    auto StdErrVNode =
        VINode::fromFd(StdErrFd, kStdErrDefaultRights, kNoInheritingRights);
    if (!StdErrVNode) {
      return WasiUnexpect(StdErrVNode.error());
    }
    FdMap.emplace(2, std::move(*StdErrVNode));

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
  return {};
}

void Environ::fini() noexcept {
  EnvironVariables.clear();
  Arguments.clear();
  FdMap.clear();
}

Environ::~Environ() noexcept { fini(); }

WasiExpect<std::shared_ptr<VINode>>
Environ::findPreopen(std::string_view &Path) const noexcept {
  // The leading "/" and "./" are not significant, as in the WASI libc.
  auto Strip = [](std::string_view Name) noexcept {
    while (true) {
      if (!Name.empty() && Name[0] == '/') {
        Name.remove_prefix(1);
      } else if (Name.size() >= 2 && Name[0] == '.' && Name[1] == '/') {
        Name.remove_prefix(2);
      } else if (Name == "."sv) {
        Name.remove_prefix(1);
      } else {
        return Name;
      }
    }
  };

  // The preopened directory with the longest name containing the path wins,
  // and the lowest file descriptor among the equal names.
  Path = Strip(Path);
  std::shared_ptr<VINode> Dir;
  __wasi_fd_t DirFd = 0;
  std::size_t DirLen = 0;
  {
    std::shared_lock Lock(FdMutex);
    for (const auto &Entry : FdMap) {
      const auto &Node = Entry.second;
      const auto Name = Strip(Node->name());
      if (Node->name().empty() ||
          (Dir && (Name.size() < DirLen ||
                   (Name.size() == DirLen && Entry.first > DirFd)))) {
        continue;
      }
      if (Name.empty() ||
          (Path.size() >= Name.size() && Path.substr(0, Name.size()) == Name &&
           (Path.size() == Name.size() || Path[Name.size()] == '/'))) {
        Dir = Node;
        DirFd = Entry.first;
        DirLen = Name.size();
      }
    }
  }
  if (!Dir) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  }
  Path = Strip(Path.substr(DirLen));
  return Dir;
}

WasiExpect<std::shared_ptr<Host::File>>
Environ::open(std::string_view Path, __wasi_oflags_t OpenFlags,
              __wasi_rights_t Rights, __wasi_fdflags_t FdFlags) const noexcept {
  if (!VINode::isPathValid(Path)) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  EXPECTED_TRY(auto Dir, findPreopen(Path));
  if (Path.empty()) {
    return WasiUnexpect(__WASI_ERRNO_ISDIR);
  }
  // The rights asked for must all be granted. Seeking is granted on top when
  // the directory allows it, so that the file is opened for the asked access.
  const auto Inheriting = VINode::imply(Dir->fsRightsInheriting());
  if ((Inheriting & Rights) != Rights) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  }
  Rights |= Inheriting & (__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
  // Creating exclusively fails on a symbolic link at the path, as open() does.
  const auto LookupFlags = (OpenFlags & __WASI_OFLAGS_EXCL) != 0
                               ? static_cast<__wasi_lookupflags_t>(0)
                               : __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
  EXPECTED_TRY(auto Node,
               VINode::pathOpen(std::move(Dir), Path, LookupFlags, OpenFlags,
                                Rights, static_cast<__wasi_rights_t>(0),
                                FdFlags));
  return std::shared_ptr<Host::File>(std::move(Node));
}

WasiExpect<void>
Environ::createDirectory(std::string_view Path) const noexcept {
  if (!VINode::isPathValid(Path)) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  EXPECTED_TRY(auto Dir, findPreopen(Path));
  // The path of a preopened directory itself names an existing directory.
  if (Path.empty()) {
    return WasiUnexpect(__WASI_ERRNO_EXIST);
  }
  return VINode::pathCreateDirectory(std::move(Dir), Path);
}

WasiExpect<bool> Environ::pathExists(std::string_view Path) const noexcept {
  __wasi_filestat_t Filestat;
  // Using the first default preopened FD 3 as the working directory.
  __wasi_fd_t BaseFd = 3;
  auto StatResult = const_cast<Environ *>(this)->pathFilestatGet(
      BaseFd, Path, static_cast<__wasi_lookupflags_t>(0), Filestat);
  if (StatResult) {
    return true;
  }
  if (StatResult.error() == __WASI_ERRNO_NOENT) {
    return false;
  }
  return WasiUnexpect(StatResult.error());
}

WasiExpect<bool> Environ::pathCanRead(std::string_view Path) const noexcept {
  // Using the first default preopened FD 3 as the working directory.
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
  // Using the first default preopened FD 3 as the working directory.
  __wasi_fd_t BaseFd = 3;
  auto Result = const_cast<Environ *>(this)->pathOpen(
      BaseFd, Path, static_cast<__wasi_lookupflags_t>(0),
      static_cast<__wasi_oflags_t>(0), __WASI_RIGHTS_FD_WRITE,
      __WASI_RIGHTS_FD_WRITE, static_cast<__wasi_fdflags_t>(0));

  if (Result) {
    __wasi_fdstat_t FdStat{};
    auto StatResult = fdFdstatGet(*Result, FdStat);
    auto CloseResult = const_cast<Environ *>(this)->fdClose(*Result);
    if (!StatResult) {
      return WasiUnexpect(StatResult.error());
    }
    if (!CloseResult) {
      return WasiUnexpect(CloseResult.error());
    }
    return (EndianValue(FdStat.fs_rights_base).le() & __WASI_RIGHTS_FD_WRITE) !=
           0;
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
