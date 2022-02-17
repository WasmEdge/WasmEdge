// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi/vinode.h"
#include "common/errcode.h"
#include "common/log.h"
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

VINode::VINode(VFS &FS, INode Node, std::shared_ptr<VINode> Parent)
    : FS(FS), Node(std::move(Node)), FsRightsBase(Parent->FsRightsBase),
      FsRightsInheriting(Parent->FsRightsInheriting),
      Parent(std::move(Parent)) {}

VINode::VINode(VFS &FS, INode Node, __wasi_rights_t FRB, __wasi_rights_t FRI,
               std::string N)
    : FS(FS), Node(std::move(Node)), FsRightsBase(FRB), FsRightsInheriting(FRI),
      Name(std::move(N)) {}

std::shared_ptr<VINode> VINode::stdIn(VFS &FS, __wasi_rights_t FRB,
                                      __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(FS, INode::stdIn(), FRB, FRI);
  return Node;
}

std::shared_ptr<VINode> VINode::stdOut(VFS &FS, __wasi_rights_t FRB,
                                       __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(FS, INode::stdOut(), FRB, FRI);
  return Node;
}

std::shared_ptr<VINode> VINode::stdErr(VFS &FS, __wasi_rights_t FRB,
                                       __wasi_rights_t FRI) {
  auto Node = std::make_shared<VINode>(FS, INode::stdErr(), FRB, FRI);
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

WasiExpect<std::shared_ptr<VINode>> VINode::bind(VFS &FS, __wasi_rights_t FRB,
                                                 __wasi_rights_t FRI,
                                                 std::string Name,
                                                 std::string SystemPath) {
  if (auto Res = INode::open(std::move(SystemPath), __WASI_OFLAGS_DIRECTORY,
                             __wasi_fdflags_t(0), VFS::Read);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    return std::make_shared<VINode>(FS, std::move(*Res), FRB, FRI,
                                    std::move(Name));
  }
}

WasiExpect<void> VINode::pathCreateDirectory(VFS &FS,
                                             std::shared_ptr<VINode> Fd,
                                             std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(FS, Fd, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_CREATE_DIRECTORY)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathCreateDirectory(std::string(Path));
}

WasiExpect<void> VINode::pathFilestatGet(VFS &FS, std::shared_ptr<VINode> Fd,
                                         std::string_view Path,
                                         __wasi_lookupflags_t Flags,
                                         __wasi_filestat_t &Filestat) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(FS, Fd, Path, Flags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_FILESTAT_GET)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathFilestatGet(std::string(Path), Filestat);
}

WasiExpect<void>
VINode::pathFilestatSetTimes(VFS &FS, std::shared_ptr<VINode> Fd,
                             std::string_view Path, __wasi_lookupflags_t Flags,
                             __wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                             __wasi_fstflags_t FstFlags) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(FS, Fd, Path, Flags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathFilestatSetTimes(std::string(Path), ATim, MTim, FstFlags);
}

WasiExpect<void> VINode::pathLink(VFS &FS, std::shared_ptr<VINode> Old,
                                  std::string_view OldPath,
                                  std::shared_ptr<VINode> New,
                                  std::string_view NewPath,
                                  __wasi_lookupflags_t LookupFlags) {
  if (unlikely(!New)) {
    return WasiUnexpect(__WASI_ERRNO_BADF);
  }

  std::vector<char> OldBuffer, NewBuffer;
  if (auto Res = resolvePath(FS, Old, OldPath, LookupFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Old->can(__WASI_RIGHTS_PATH_LINK_SOURCE)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    OldBuffer = std::move(*Res);
  }

  if (auto Res = resolvePath(FS, New, NewPath, LookupFlags); unlikely(!Res)) {
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
VINode::pathOpen(VFS &FS, std::shared_ptr<VINode> Fd, std::string_view Path,
                 __wasi_lookupflags_t LookupFlags, __wasi_oflags_t OpenFlags,
                 __wasi_rights_t FsRightsBase,
                 __wasi_rights_t FsRightsInheriting, __wasi_fdflags_t FdFlags) {
  std::vector<char> Buffer;

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
    RequiredRights |= __WASI_RIGHTS_FD_SYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    RequiredRights |= __WASI_RIGHTS_FD_DATASYNC;
  }

  if (auto Res = resolvePath(FS, Fd, Path, LookupFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(RequiredRights, RequiredInheritingRights)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }
  uint8_t VFSFlags = 0;
  if (Read) {
    VFSFlags |= VFS::Read;
  }
  if (Write) {
    VFSFlags |= VFS::Write;
  }
  return Fd->directOpen(Path, OpenFlags, FdFlags, VFSFlags);
}

WasiExpect<void> VINode::pathReadlink(VFS &FS, std::shared_ptr<VINode> Fd,
                                      std::string_view Path, Span<char> Buffer,
                                      __wasi_size_t &NRead) {
  std::vector<char> PathBuffer;
  if (auto Res = resolvePath(FS, Fd, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_READLINK)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    PathBuffer = std::move(*Res);
  }

  return Fd->Node.pathReadlink(std::string(Path), Buffer, NRead);
}

WasiExpect<void> VINode::pathRemoveDirectory(VFS &FS,
                                             std::shared_ptr<VINode> Fd,
                                             std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res = resolvePath(FS, Fd, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Fd->can(__WASI_RIGHTS_PATH_REMOVE_DIRECTORY)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    Buffer = std::move(*Res);
  }

  return Fd->Node.pathRemoveDirectory(std::string(Path));
}

WasiExpect<void> VINode::pathRename(VFS &FS, std::shared_ptr<VINode> Old,
                                    std::string_view OldPath,
                                    std::shared_ptr<VINode> New,
                                    std::string_view NewPath) {
  std::vector<char> OldBuffer, NewBuffer;
  if (auto Res = resolvePath(FS, Old, OldPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!Old->can(__WASI_RIGHTS_PATH_RENAME_SOURCE)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    OldBuffer = std::move(*Res);
  }
  if (auto Res = resolvePath(FS, New, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!New->can(__WASI_RIGHTS_PATH_RENAME_TARGET)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    NewBuffer = std::move(*Res);
  }

  return INode::pathRename(Old->Node, std::string(OldPath), New->Node,
                           std::string(NewPath));
}

WasiExpect<void> VINode::pathSymlink(VFS &FS, std::string_view OldPath,
                                     std::shared_ptr<VINode> New,
                                     std::string_view NewPath) {
  if (unlikely(!New)) {
    return WasiUnexpect(__WASI_ERRNO_BADF);
  }

  std::vector<char> NewBuffer;
  if (auto Res = resolvePath(FS, New, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (!New->can(__WASI_RIGHTS_PATH_SYMLINK)) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  } else {
    NewBuffer = std::move(*Res);
  }

  return New->Node.pathSymlink(std::string(OldPath), std::string(NewPath));
}

WasiExpect<void> VINode::pathUnlinkFile(VFS &FS, std::shared_ptr<VINode> Fd,
                                        std::string_view Path) {
  std::vector<char> Buffer;
  if (auto Res =
          resolvePath(FS, Fd, Path, static_cast<__wasi_lookupflags_t>(0));
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
VINode::sockOpen(VFS &FS, __wasi_address_family_t SysDomain,
                 __wasi_sock_type_t SockType) {
  if (auto Res = INode::sockOpen(SysDomain, SockType); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    __wasi_rights_t Rights =
        __WASI_RIGHTS_SOCK_OPEN | __WASI_RIGHTS_SOCK_CLOSE |
        __WASI_RIGHTS_SOCK_RECV | __WASI_RIGHTS_SOCK_RECV_FROM |
        __WASI_RIGHTS_SOCK_SEND | __WASI_RIGHTS_SOCK_SEND_TO |
        __WASI_RIGHTS_SOCK_SHUTDOWN | __WASI_RIGHTS_SOCK_BIND |
        __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS;
    return std::make_shared<VINode>(FS, std::move(*Res), Rights, Rights);
  }
}

WasiExpect<std::shared_ptr<VINode>> VINode::sockAccept() {
  if (auto Res = Node.sockAccept(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    __wasi_rights_t Rights =
        __WASI_RIGHTS_SOCK_RECV | __WASI_RIGHTS_SOCK_RECV_FROM |
        __WASI_RIGHTS_SOCK_SEND | __WASI_RIGHTS_SOCK_SEND_TO |
        __WASI_RIGHTS_SOCK_SHUTDOWN | __WASI_RIGHTS_POLL_FD_READWRITE |
        __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS;
    return std::make_shared<VINode>(FS, std::move(*Res), Rights, Rights,
                                    std::string());
  }
}

WasiExpect<std::shared_ptr<VINode>>
VINode::directOpen(std::string_view Path, __wasi_oflags_t OpenFlags,
                   __wasi_fdflags_t FdFlags, uint8_t VFSFlags) {
  std::string PathStr(Path);

  if (auto Res =
          Node.pathOpen(std::move(PathStr), OpenFlags, FdFlags, VFSFlags);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    return std::make_shared<VINode>(FS, std::move(*Res), shared_from_this());
  }
}

WasiExpect<std::vector<char>>
VINode::resolvePath(VFS &FS, std::shared_ptr<VINode> &Fd,
                    std::string_view &Path, __wasi_lookupflags_t LookupFlags,
                    uint8_t VFSFlags, uint8_t LinkCount) {
  std::vector<char> Buffer;
  do {
    // check empty path
    if (Path.empty() && (VFSFlags & VFS::AllowEmpty) == 0) {
      return WasiUnexpect(__WASI_ERRNO_NOENT);
    }

    // check absolute path
    {
      const bool IsEmpty = Path.empty();
      const bool IsAbsolute = !IsEmpty && Path[0] == '/';
      if (unlikely(!Fd && (IsEmpty || !IsAbsolute))) {
        return WasiUnexpect(__WASI_ERRNO_BADF);
      }

      if (IsAbsolute) {
        VFSFlags |= VFS::AllowEmpty;
        while (Fd->Parent) {
          Fd = Fd->Parent;
        }
        do {
          Path = Path.substr(1);
        } while (!Path.empty() && Path[0] == '/');
      }
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
      const bool LastPart = Remain.empty() && Slash == std::string_view::npos;

      if (!Part.empty() && Part[0] == '.') {
        if (Part.size() == 1) {
          if (LastPart) {
            return Buffer;
          }
          Path = Remain;
          continue;
        }
        if (Part.size() == 2 && Part[1] == '.') {
          if (Fd->Parent) {
            Fd = Fd->Parent;
          } else {
            return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
          }
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

        std::vector<char> NewBuffer(Filestat.size);
        __wasi_size_t NRead;
        if (auto Res =
                Fd->Node.pathReadlink(std::string(Part), NewBuffer, NRead);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        } else {
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

      if (auto Child = Fd->Node.pathOpen(
              std::string(Part), static_cast<__wasi_oflags_t>(0),
              static_cast<__wasi_fdflags_t>(0), VFSFlags);
          unlikely(!Child)) {
        return WasiUnexpect(Child);
      } else {
        // fast retry
        Fd = std::make_shared<VINode>(FS, std::move(*Child), Fd);
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
