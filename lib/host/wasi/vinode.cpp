// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "host/wasi/vinode.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "host/wasi/environ.h"
#include "host/wasi/vfs.h"
#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>

using namespace std::literals;

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {

static inline constexpr const uint8_t kMaxNestedLinks = 8;

}

VINode::VINode(INode Node, __wasi_rights_t FRB, __wasi_rights_t FRI,
               std::string N)
    : Node(std::move(Node)), FsRightsBase(FRB), FsRightsInheriting(FRI),
      Name(std::move(N)) {}

std::shared_ptr<VINode> VINode::stdIn(__wasi_rights_t FRB,
                                      __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(INode::stdIn(), FRB, FRI);
  return Node;
}

std::shared_ptr<VINode> VINode::stdOut(__wasi_rights_t FRB,
                                       __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(INode::stdOut(), FRB, FRI);
  return Node;
}

std::shared_ptr<VINode> VINode::stdErr(__wasi_rights_t FRB,
                                       __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(INode::stdErr(), FRB, FRI);
  return Node;
}

std::string VINode::canonicalGuest(std::string_view Path) {
  std::vector<std::string_view> Parts;

  while (!Path.empty() && Path.front() == '/') {
    Path = Path.substr(1);
  }
  while (!Path.empty()) {
    auto Slash = Path.find('/');
    const auto Part = Path.substr(0, Slash);
    auto Remain = Path.substr(Part.size());
    while (!Remain.empty() && Remain.front() == '/') {
      Remain = Remain.substr(1);
    }
    if (Part.front() == '.' && Part.size() == 2 && Part[1] == '.') {
      if (!Parts.empty()) {
        Parts.pop_back();
      }
    } else if (Part.front() != '.' || Parts.size() != 1) {
      Parts.push_back(Part);
    }
    if (Remain.empty()) {
      break;
    }
    Path = Remain;
  }
  if (Parts.empty()) {
    Parts.push_back({});
  }

  std::string Result;
  Result.reserve(std::accumulate(
      Parts.begin(), Parts.end(), Parts.size(),
      [](size_t L, std::string_view P) { return L + P.size(); }));
  std::for_each(Parts.begin(), Parts.end(), [&Result](std::string_view P) {
    Result += P;
    Result += '/';
  });
  if (!Result.empty()) {
    Result.pop_back();
  }

  return Result;
}

WasiExpect<std::shared_ptr<VINode>> VINode::bind(__wasi_rights_t FRB,
                                                 __wasi_rights_t FRI,
                                                 std::string Name,
                                                 std::string SystemPath) {
  if (auto Res = INode::open(std::move(SystemPath), __WASI_OFLAGS_DIRECTORY,
                             __wasi_fdflags_t(0), VFS::Read);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    return std::make_shared<VINode>(std::move(*Res), FRB, FRI, std::move(Name));
  }
}

WasiExpect<void> VINode::pathCreateDirectory(std::shared_ptr<VINode> Fd,
                                             std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, false); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_CREATE_DIRECTORY)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathCreateDirectory(std::string(Path));
}

WasiExpect<void> VINode::pathFilestatGet(std::shared_ptr<VINode> Fd,
                                         std::string_view Path,
                                         __wasi_lookupflags_t Flags,
                                         __wasi_filestat_t &Filestat) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, Flags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_FILESTAT_GET)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathFilestatGet(std::string(Path), Filestat);
}

WasiExpect<void> VINode::pathFilestatSetTimes(std::shared_ptr<VINode> Fd,
                                              std::string_view Path,
                                              __wasi_lookupflags_t Flags,
                                              __wasi_timestamp_t ATim,
                                              __wasi_timestamp_t MTim,
                                              __wasi_fstflags_t FstFlags) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, Flags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathFilestatSetTimes(std::string(Path), ATim, MTim, FstFlags);
}

WasiExpect<void> VINode::pathLink(std::shared_ptr<VINode> Old,
                                  std::string_view OldPath,
                                  std::shared_ptr<VINode> New,
                                  std::string_view NewPath,
                                  __wasi_lookupflags_t LookupFlags) {
  if (unlikely(!New)) {
    return WasiUnexpect(__WASI_ERRNO_BADF);
  }

  std::vector<char> OldBuffer, NewBuffer;
  if (auto Res = resolvePath(Old, OldPath, LookupFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Old->can(__WASI_RIGHTS_PATH_LINK_SOURCE)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    OldBuffer = std::move(*Res);
  }

  if (auto Res = resolvePath(New, NewPath, LookupFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!New->can(__WASI_RIGHTS_PATH_LINK_TARGET)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    NewBuffer = std::move(*Res);
  }

  return INode::pathLink(Old->Node, std::string(OldPath), New->Node,
                         std::string(NewPath));
}

WasiExpect<std::shared_ptr<VINode>>
VINode::pathOpen(std::shared_ptr<VINode> Fd, std::string_view Path,
                 __wasi_lookupflags_t LookupFlags, __wasi_oflags_t OpenFlags,
                 __wasi_rights_t FsRightsBase,
                 __wasi_rights_t FsRightsInheriting, __wasi_fdflags_t FdFlags) {
  if (OpenFlags & __WASI_OFLAGS_DIRECTORY) {
    FsRightsBase &= ~__WASI_RIGHTS_FD_SEEK;
  } else {
    FsRightsBase &= ~__WASI_RIGHTS_PATH_FILESTAT_GET;
    FsRightsInheriting &= ~__WASI_RIGHTS_PATH_FILESTAT_GET;
  }

  __wasi_rights_t RequiredRights = __WASI_RIGHTS_PATH_OPEN;
  __wasi_rights_t RequiredInheritingRights = FsRightsBase | FsRightsInheriting;
  const bool Read =
      (FsRightsBase & (__WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR)) != 0;
  const bool Write =
      (FsRightsBase &
       (__WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_WRITE |
        __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE)) != 0;

  if (OpenFlags & __WASI_OFLAGS_CREAT) {
    RequiredRights |= __WASI_RIGHTS_PATH_CREATE_FILE;
  }
  if (OpenFlags & __WASI_OFLAGS_TRUNC) {
    RequiredRights |= __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE;
  }
  if (FdFlags & __WASI_FDFLAGS_RSYNC) {
    RequiredInheritingRights |= __WASI_RIGHTS_FD_SYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    RequiredInheritingRights |= __WASI_RIGHTS_FD_DATASYNC;
  }

  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, LookupFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(RequiredRights, RequiredInheritingRights)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }
  VFS::Flags VFSFlags = static_cast<VFS::Flags>(0);
  if (Read) {
    VFSFlags |= VFS::Read;
  }
  if (Write) {
    VFSFlags |= VFS::Write;
  }
  return Fd->directOpen(Path, OpenFlags, FdFlags, VFSFlags, FsRightsBase,
                        FsRightsInheriting);
}

WasiExpect<void> VINode::pathReadlink(std::shared_ptr<VINode> Fd,
                                      std::string_view Path, Span<char> Buffer,
                                      __wasi_size_t &NRead) {
  std::vector<char> PathBuffer;
  if (auto Res = resolvePath(Fd, Path, static_cast<__wasi_lookupflags_t>(0));
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_READLINK)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    PathBuffer = std::move(*Res);
  }

  return Fd->Node.pathReadlink(std::string(Path), Buffer, NRead);
}

WasiExpect<void> VINode::pathRemoveDirectory(std::shared_ptr<VINode> Fd,
                                             std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, false); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_REMOVE_DIRECTORY)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathRemoveDirectory(std::string(Path));
}

WasiExpect<void> VINode::pathRename(std::shared_ptr<VINode> Old,
                                    std::string_view OldPath,
                                    std::shared_ptr<VINode> New,
                                    std::string_view NewPath) {
  std::vector<char> OldBuffer, NewBuffer;
  if (auto Res = resolvePath(Old, OldPath, false); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Old->can(__WASI_RIGHTS_PATH_RENAME_SOURCE)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    OldBuffer = std::move(*Res);
  }
  if (auto Res = resolvePath(New, NewPath, false); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!New->can(__WASI_RIGHTS_PATH_RENAME_TARGET)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    NewBuffer = std::move(*Res);
  }

  return INode::pathRename(Old->Node, std::string(OldPath), New->Node,
                           std::string(NewPath));
}

WasiExpect<void> VINode::pathSymlink(std::string_view OldPath,
                                     std::shared_ptr<VINode> New,
                                     std::string_view NewPath) {
  if (unlikely(!New)) {
    return WasiUnexpect(__WASI_ERRNO_BADF);
  }

  std::vector<char> NewBuffer;
  if (auto Res = resolvePath(New, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!New->can(__WASI_RIGHTS_PATH_SYMLINK)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    NewBuffer = std::move(*Res);
  }

  return New->Node.pathSymlink(std::string(OldPath), std::string(NewPath));
}

WasiExpect<void> VINode::pathUnlinkFile(std::shared_ptr<VINode> Fd,
                                        std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(Fd, Path, static_cast<__wasi_lookupflags_t>(0));
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_UNLINK_FILE)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathUnlinkFile(std::string(Path));
}

WasiExpect<void>
VINode::getAddrinfo(std::string_view Node, std::string_view Service,
                    const __wasi_addrinfo_t &Hint, uint32_t MaxResLength,
                    Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                    Span<__wasi_sockaddr_t *> WasiSockaddrArray,
                    Span<char *> AiAddrSaDataArray,
                    Span<char *> AiCanonnameArray,
                    /*Out*/ __wasi_size_t &ResLength) noexcept {
  if (auto Res = INode::getAddrinfo(
          Node, Service, Hint, MaxResLength, WasiAddrinfoArray,
          WasiSockaddrArray, AiAddrSaDataArray, AiCanonnameArray, ResLength);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  }
  return {};
}

WasiExpect<std::shared_ptr<VINode>>
VINode::sockOpen(__wasi_address_family_t SysDomain,
                 __wasi_sock_type_t SockType) {
  if (auto Res = INode::sockOpen(SysDomain, SockType); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    __wasi_rights_t Rights =
        __WASI_RIGHTS_SOCK_OPEN | __WASI_RIGHTS_SOCK_CLOSE |
        __WASI_RIGHTS_SOCK_RECV | __WASI_RIGHTS_SOCK_RECV_FROM |
        __WASI_RIGHTS_SOCK_SEND | __WASI_RIGHTS_SOCK_SEND_TO |
        __WASI_RIGHTS_SOCK_SHUTDOWN | __WASI_RIGHTS_SOCK_BIND |
        __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
        __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_WRITE;
    return std::make_shared<VINode>(std::move(*Res), Rights, Rights);
  }
}

WasiExpect<std::shared_ptr<VINode>>
VINode::sockAccept(__wasi_fdflags_t FdFlags) {
  if (auto Res = Node.sockAccept(FdFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    __wasi_rights_t Rights =
        __WASI_RIGHTS_SOCK_RECV | __WASI_RIGHTS_SOCK_RECV_FROM |
        __WASI_RIGHTS_SOCK_SEND | __WASI_RIGHTS_SOCK_SEND_TO |
        __WASI_RIGHTS_SOCK_SHUTDOWN | __WASI_RIGHTS_POLL_FD_READWRITE |
        __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS | __WASI_RIGHTS_FD_READ |
        __WASI_RIGHTS_FD_WRITE;
    return std::make_shared<VINode>(std::move(*Res), Rights, Rights,
                                    std::string());
  }
}

WasiExpect<std::shared_ptr<VINode>>
VINode::directOpen(std::string_view Path, __wasi_oflags_t OpenFlags,
                   __wasi_fdflags_t FdFlags, VFS::Flags VFSFlags,
                   __wasi_rights_t RightsBase,
                   __wasi_rights_t RightsInheriting) {
  std::string PathStr(Path);

  if (auto Res =
          Node.pathOpen(std::move(PathStr), OpenFlags, FdFlags, VFSFlags);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    return std::make_shared<VINode>(std::move(*Res), RightsBase,
                                    RightsInheriting);
  }
}

WasiExpect<std::vector<char>>
VINode::resolvePath(std::shared_ptr<VINode> &Fd, std::string_view &Path,
                    __wasi_lookupflags_t LookupFlags, VFS::Flags VFSFlags,
                    uint8_t LinkCount, bool FollowTrailingSlashes) {
  std::vector<std::shared_ptr<VINode>> PartFds;
  std::vector<char> Buffer;
  do {
    // check empty path
    if (Path.empty() && (VFSFlags & VFS::AllowEmpty) == 0) {
      return WasiUnexpect(__WASI_ERRNO_NOENT);
    }

    // check absolute path
    if (!Path.empty() && Path[0] == '/') {
      return WasiUnexpect(__WASI_ERRNO_PERM);
    }

    if (!Fd) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    }

    if (!Fd->isDirectory()) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    if (!Fd->canBrowse()) {
      return WasiUnexpect(__WASI_ERRNO_ACCES);
    }

    do {
      // check self type
      auto Slash = Path.find('/');
      const auto Part = Path.substr(0, Slash);
      auto Remain = Path.substr(Part.size());
      while (!Remain.empty() && Remain[0] == '/') {
        Remain = Remain.substr(1);
      }
      const bool LastPart = Remain.empty() && (!FollowTrailingSlashes ||
                                               Slash == std::string_view::npos);

      if (!Part.empty() && Part[0] == '.') {
        if (Part.size() == 1) {
          if (LastPart) {
            return Buffer;
          }
          Path = Remain;
          continue;
        }
        if (Part.size() == 2 && Part[1] == '.') {
          if (PartFds.empty()) {
            return WasiUnexpect(__WASI_ERRNO_PERM);
          }
          Fd = std::move(PartFds.back());
          PartFds.pop_back();
          Path = Remain;
          if (LastPart) {
            Path = "."sv;
            return Buffer;
          }
          continue;
        }
      }

      if (LastPart && !(LookupFlags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW)) {
        Path = Part;
        return Buffer;
      }

      __wasi_filestat_t Filestat;
      if (auto Res = Fd->Node.pathFilestatGet(std::string(Part), Filestat);
          unlikely(!Res)) {
        if (LastPart) {
          Path = Part;
          return Buffer;
        }
        return WasiUnexpect(Res);
      }

      if (Filestat.filetype == __WASI_FILETYPE_SYMBOLIC_LINK) {
        if (++LinkCount >= kMaxNestedLinks) {
          return WasiUnexpect(__WASI_ERRNO_LOOP);
        }

        std::vector<char> NewBuffer(16384);
        __wasi_size_t NRead;
        if (auto Res =
                Fd->Node.pathReadlink(std::string(Part), NewBuffer, NRead);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        } else {
          NewBuffer.resize(NRead);
          // Don't drop Buffer now because Path may referencing it.
          if (!Remain.empty()) {
            if (NewBuffer.back() != '/') {
              NewBuffer.push_back('/');
            }
            NewBuffer.insert(NewBuffer.end(), Remain.begin(), Remain.end());
          }
          // slow retry
          Buffer = std::move(NewBuffer);
          Path = std::string_view(Buffer.data(), Buffer.size());
          break;
        }
      }

      if (LastPart) {
        Path = Part;
        return Buffer;
      }

      if (Filestat.filetype != __WASI_FILETYPE_DIRECTORY) {
        return WasiUnexpect(__WASI_ERRNO_NOTDIR);
      }

      if (auto Child =
              Fd->Node.pathOpen(std::string(Part), __WASI_OFLAGS_DIRECTORY,
                                static_cast<__wasi_fdflags_t>(0), VFSFlags);
          unlikely(!Child)) {
        return WasiUnexpect(Child);
      } else {
        // fast retry
        PartFds.push_back(std::exchange(
            Fd, std::make_shared<VINode>(std::move(*Child), Fd->FsRightsBase,
                                         Fd->FsRightsInheriting)));
        Path = Remain;
        if (Path.empty()) {
          Path = "."sv;
          return {};
        }
        continue;
      }
    } while (true);
  } while (true);
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
