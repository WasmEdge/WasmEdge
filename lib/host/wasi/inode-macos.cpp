// SPDX-License-Identifier: Apache-2.0
#include "common/defines.h"
#if WASMEDGE_OS_MACOS

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "macos.h"

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {

inline constexpr bool isSpecialFd(int Fd) noexcept {
  switch (Fd) {
  case STDIN_FILENO:
  case STDOUT_FILENO:
  case STDERR_FILENO:
    return true;
  default:
    return false;
  }
}

constexpr int openFlags(__wasi_oflags_t OpenFlags, __wasi_fdflags_t FdFlags,
                        uint8_t VFSFlags) noexcept {
  int Flags = O_CLOEXEC | O_NOFOLLOW;

  if (VFSFlags & VFS::Read) {
    if (VFSFlags & VFS::Write) {
      Flags |= O_RDWR;
    } else {
      Flags |= O_RDONLY;
    }
  } else if (VFSFlags & VFS::Write) {
    Flags |= O_WRONLY;
  } else {
    Flags |= O_RDONLY;
  }

  if (OpenFlags & __WASI_OFLAGS_CREAT) {
    Flags |= O_CREAT;
  }
  if (OpenFlags & __WASI_OFLAGS_DIRECTORY) {
    Flags |= O_DIRECTORY;
  }
  if (OpenFlags & __WASI_OFLAGS_EXCL) {
    Flags |= O_EXCL;
  }
  if (OpenFlags & __WASI_OFLAGS_TRUNC) {
    Flags |= O_TRUNC;
  }

  // Convert file descriptor flags.
  if ((FdFlags & __WASI_FDFLAGS_APPEND) != 0) {
    Flags |= O_APPEND;
  }
  if ((FdFlags & (__WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC |
                  __WASI_FDFLAGS_SYNC)) != 0) {
    Flags |= O_SYNC;
  }
  if ((FdFlags & __WASI_FDFLAGS_NONBLOCK) != 0) {
    Flags |= O_NONBLOCK;
  }

  return Flags;
}

} // namespace

void FdHolder::reset() noexcept {
  if (likely(ok())) {
    if (likely(!isSpecialFd(Fd))) {
      close(Fd);
    }
    Fd = -1;
  }
}

void DirHolder::reset() noexcept {
  if (likely(Dir != nullptr)) {
    closedir(Dir);
    Dir = nullptr;
    Cookie = 0;
  }
}

INode INode::stdIn() noexcept { return INode(STDIN_FILENO); }

INode INode::stdOut() noexcept { return INode(STDOUT_FILENO); }

INode INode::stdErr() noexcept { return INode(STDERR_FILENO); }

WasiExpect<INode> INode::open(std::string Path, __wasi_oflags_t OpenFlags,
                              __wasi_fdflags_t FdFlags,
                              uint8_t VFSFlags) noexcept {
  const int Flags = openFlags(OpenFlags, FdFlags, VFSFlags);

  if (auto NewFd = ::open(Path.c_str(), Flags, 0644); unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    return INode(NewFd);
  }
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t, __wasi_filesize_t,
                                 __wasi_advice_t) const noexcept {
  /// Not supported, just ignore it.
  return {};
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t Offset,
                                   __wasi_filesize_t Len) const noexcept {
  if (Len > std::numeric_limits<int64_t>::max()) {
    return WasiUnexpect(__WASI_ERRNO_NOSPC);
  }
  const auto OldOffset = ::lseek(Fd, 0, SEEK_CUR);
  if (OldOffset < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  const auto EofOffset = ::lseek(Fd, 0, SEEK_END);
  if (EofOffset < 0 || ::lseek(Fd, OldOffset, SEEK_SET) < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (Len <= static_cast<__wasi_filesize_t>(EofOffset) &&
      Offset <= static_cast<__wasi_filesize_t>(EofOffset) - Len) {
    /// File is already large enough.
    return {};
  }

  /// Try to allocate contiguous space.
  fstore_t Store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0,
                    static_cast<int64_t>(Len), 0};
  if (auto Res = ::fcntl(Fd, F_PREALLOCATE, &Store); unlikely(Res < 0)) {
    /// Try to allocate sparse space.
    Store.fst_flags = F_ALLOCATEALL;
    if (auto Res = ::fcntl(Fd, F_PREALLOCATE, &Store); unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }
  if (auto Res = ::ftruncate(Fd, Offset + Len); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept {
  if (auto Res = ::fcntl(Fd, F_FULLFSYNC); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept {
  if (auto Res = updateStat(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  if (int FdFlags = ::fcntl(Fd, F_GETFL); unlikely(FdFlags < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    FdStat.fs_filetype = unsafeFiletype();

    FdStat.fs_flags = static_cast<__wasi_fdflags_t>(0);
    if (FdFlags & O_APPEND) {
      FdStat.fs_flags |= __WASI_FDFLAGS_APPEND;
    }
    if (FdFlags & O_DSYNC) {
      FdStat.fs_flags |= __WASI_FDFLAGS_DSYNC;
    }
    if (FdFlags & O_NONBLOCK) {
      FdStat.fs_flags |= __WASI_FDFLAGS_NONBLOCK;
    }
    if (FdFlags & O_SYNC) {
      FdStat.fs_flags |= __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;
    }
  }

  return {};
}

WasiExpect<void>
INode::fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept {
  int SysFlag = 0;
  if (FdFlags & __WASI_FDFLAGS_NONBLOCK) {
    SysFlag |= O_NONBLOCK;
  }
  if (FdFlags & __WASI_FDFLAGS_APPEND) {
    SysFlag |= O_APPEND;
  }
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    SysFlag |= O_DSYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_RSYNC) {
    SysFlag |= O_SYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_SYNC) {
    SysFlag |= O_SYNC;
  }

  if (auto Res = ::fcntl(Fd, F_SETFL, SysFlag); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void>
INode::fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept {
  if (auto Res = updateStat(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  Filestat.dev = Stat->st_dev;
  Filestat.ino = Stat->st_ino;
  Filestat.filetype = unsafeFiletype();
  Filestat.nlink = Stat->st_nlink;
  Filestat.size = Stat->st_size;
  Filestat.atim = fromTimespec(Stat->st_atimespec);
  Filestat.mtim = fromTimespec(Stat->st_mtimespec);
  Filestat.ctim = fromTimespec(Stat->st_ctimespec);

  return {};
}

WasiExpect<void>
INode::fdFilestatSetSize(__wasi_filesize_t Size) const noexcept {
  if (auto Res = ::ftruncate(Fd, Size); unlikely(Res == -1)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void>
INode::fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                          __wasi_fstflags_t FstFlags) const noexcept {
  if (available(10, 13, 0, 11, 0, 0, 11, 0, 0, 4, 0, 0)) {
    timespec SysTimespec[2];
    if (FstFlags & __WASI_FSTFLAGS_ATIM) {
      SysTimespec[0] = toTimespec(ATim);
    } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
      SysTimespec[0].tv_nsec = UTIME_NOW;
    } else {
      SysTimespec[0].tv_nsec = UTIME_OMIT;
    }
    if (FstFlags & __WASI_FSTFLAGS_MTIM) {
      SysTimespec[1] = toTimespec(MTim);
    } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
      SysTimespec[1].tv_nsec = UTIME_NOW;
    } else {
      SysTimespec[1].tv_nsec = UTIME_OMIT;
    }

    if (auto Res = ::futimens(Fd, SysTimespec); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }

    return {};
  }

  bool NeedNow = false;
  bool NeedFile = false;
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }

  if (NeedFile) {
    if (auto Res = updateStat(); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
  }

  timespec Now;
  if (NeedNow) {
    if (auto Res = ::clock_gettime(CLOCK_REALTIME, &Now); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  timeval SysTimeval[2];
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    SysTimeval[0] = toTimeval(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    SysTimeval[0] = toTimeval(Now);
  } else {
    SysTimeval[0] = toTimeval(Stat->st_atimespec);
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimeval[1] = toTimeval(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimeval[1] = toTimeval(Now);
  } else {
    SysTimeval[1] = toTimeval(Stat->st_mtimespec);
  }

  if (auto Res = ::futimes(Fd, SysTimeval); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdPread(Span<Span<uint8_t>> IOVs,
                                __wasi_filesize_t Offset,
                                __wasi_size_t &NRead) const noexcept {
  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : IOVs) {
    SysIOVs[SysIOVsSize].iov_base = IOV.data();
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  const auto OldOffset = ::lseek(Fd, 0, SEEK_CUR);
  if (OldOffset < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (::lseek(Fd, Offset, SEEK_SET) < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (auto Res = ::readv(Fd, SysIOVs, SysIOVsSize); unlikely(Res < 0)) {
    ::lseek(Fd, OldOffset, SEEK_SET);
    return WasiUnexpect(fromErrNo(errno));
  } else {
    if (::lseek(Fd, OldOffset, SEEK_SET) < 0) {
      return WasiUnexpect(fromErrNo(errno));
    }
    NRead = Res;
  }

  return {};
}

WasiExpect<void> INode::fdPwrite(Span<Span<const uint8_t>> IOVs,
                                 __wasi_filesize_t Offset,
                                 __wasi_size_t &NWritten) const noexcept {
  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : IOVs) {
    SysIOVs[SysIOVsSize].iov_base = const_cast<uint8_t *>(IOV.data());
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  const auto OldOffset = ::lseek(Fd, 0, SEEK_CUR);
  if (OldOffset < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (::lseek(Fd, Offset, SEEK_SET) < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (auto Res = ::writev(Fd, SysIOVs, SysIOVsSize); unlikely(Res < 0)) {
    ::lseek(Fd, OldOffset, SEEK_SET);
    return WasiUnexpect(fromErrNo(errno));
  } else {
    if (::lseek(Fd, OldOffset, SEEK_SET) < 0) {
      return WasiUnexpect(fromErrNo(errno));
    }
    NWritten = Res;
  }

  return {};
}

WasiExpect<void> INode::fdRead(Span<Span<uint8_t>> IOVs,
                               __wasi_size_t &NRead) const noexcept {
  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : IOVs) {
    SysIOVs[SysIOVsSize].iov_base = IOV.data();
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  if (auto Res = ::readv(Fd, SysIOVs, SysIOVsSize); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NRead = Res;
  }

  return {};
}

WasiExpect<void> INode::fdReaddir(Span<uint8_t> Buffer,
                                  __wasi_dircookie_t Cookie,
                                  __wasi_size_t &Size) noexcept {
  if (unlikely(!Dir.ok())) {
    if (FdHolder NewFd(::dup(Fd)); unlikely(!NewFd.ok())) {
      return WasiUnexpect(fromErrNo(errno));
    } else if (DIR *D = ::fdopendir(NewFd.Fd); unlikely(!D)) {
      return WasiUnexpect(fromErrNo(errno));
    } else {
      NewFd.release();
      Dir.emplace(D);
    }
  }

  if (unlikely(Cookie != Dir.Cookie)) {
    Dir.Buffer.clear();
    seekdir(Dir.Dir, Cookie);
  }

  Size = 0;
  do {
    if (!Dir.Buffer.empty()) {
      const auto NewDataSize =
          std::min<uint32_t>(Buffer.size(), Dir.Buffer.size());
      std::copy(Dir.Buffer.begin(), Dir.Buffer.begin() + NewDataSize,
                Buffer.begin());
      Buffer = Buffer.subspan(NewDataSize);
      Size += NewDataSize;
      Dir.Buffer.erase(Dir.Buffer.begin(), Dir.Buffer.begin() + NewDataSize);
      if (unlikely(Buffer.empty())) {
        break;
      }
    }
    errno = 0;
    dirent *SysDirent = ::readdir(Dir.Dir);
    if (SysDirent == nullptr) {
      if (errno != 0) {
        return WasiUnexpect(fromErrNo(errno));
      }
      // End of entries
      break;
    }
    Dir.Cookie = SysDirent->d_seekoff;
    std::string_view Name = SysDirent->d_name;

    Dir.Buffer.resize(sizeof(__wasi_dirent_t) + Name.size());

    __wasi_dirent_t *const Dirent =
        reinterpret_cast<__wasi_dirent_t *>(Dir.Buffer.data());
    Dirent->d_next = Dir.Cookie;
    Dirent->d_ino = SysDirent->d_ino;
    Dirent->d_type = fromFileType(SysDirent->d_type);
    Dirent->d_namlen = Name.size();
    std::copy(Name.cbegin(), Name.cend(),
              Dir.Buffer.begin() + sizeof(__wasi_dirent_t));
  } while (!Buffer.empty());

  return {};
}

WasiExpect<void> INode::fdSeek(__wasi_filedelta_t Offset,
                               __wasi_whence_t Whence,
                               __wasi_filesize_t &Size) const noexcept {
  if (auto Res = ::lseek(Fd, Offset, toWhence(Whence)); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    Size = Res;
  }

  return {};
}

WasiExpect<void> INode::fdSync() const noexcept {
  if (auto Res = ::fsync(Fd); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdTell(__wasi_filesize_t &Size) const noexcept {
  if (auto Res = ::lseek(Fd, 0, SEEK_CUR); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    Size = Res;
  }

  return {};
}

WasiExpect<void> INode::fdWrite(Span<Span<const uint8_t>> IOVs,
                                __wasi_size_t &NWritten) const noexcept {
  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : IOVs) {
    SysIOVs[SysIOVsSize].iov_base = const_cast<uint8_t *>(IOV.data());
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  if (auto Res = ::writev(Fd, SysIOVs, SysIOVsSize); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NWritten = Res;
  }

  return {};
}

WasiExpect<void> INode::pathCreateDirectory(std::string Path) const noexcept {
  if (auto Res = ::mkdirat(Fd, Path.c_str(), 0755); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void>
INode::pathFilestatGet(std::string Path,
                       __wasi_filestat_t &Filestat) const noexcept {
  struct stat SysFStat;
  if (int Res = ::fstatat(Fd, Path.c_str(), &SysFStat, AT_SYMLINK_NOFOLLOW);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  Filestat.dev = SysFStat.st_dev;
  Filestat.ino = SysFStat.st_ino;
  Filestat.filetype = fromFileType(SysFStat.st_mode);
  Filestat.nlink = SysFStat.st_nlink;
  Filestat.size = SysFStat.st_size;
  Filestat.atim = fromTimespec(SysFStat.st_atimespec);
  Filestat.mtim = fromTimespec(SysFStat.st_mtimespec);
  Filestat.ctim = fromTimespec(SysFStat.st_ctimespec);

  return {};
}

WasiExpect<void>
INode::pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                            __wasi_timestamp_t MTim,
                            __wasi_fstflags_t FstFlags) const noexcept {
  if (available(10, 13, 0, 11, 0, 0, 11, 0, 0, 4, 0, 0)) {
    timespec SysTimespec[2];
    if (FstFlags & __WASI_FSTFLAGS_ATIM) {
      SysTimespec[0] = toTimespec(ATim);
    } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
      SysTimespec[0].tv_nsec = UTIME_NOW;
    } else {
      SysTimespec[0].tv_nsec = UTIME_OMIT;
    }
    if (FstFlags & __WASI_FSTFLAGS_MTIM) {
      SysTimespec[1] = toTimespec(MTim);
    } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
      SysTimespec[1].tv_nsec = UTIME_NOW;
    } else {
      SysTimespec[1].tv_nsec = UTIME_OMIT;
    }

    if (auto Res = ::utimensat(Fd, Path.c_str(), SysTimespec, 0);
        unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }

    return {};
  }

  bool NeedNow = false;
  bool NeedFile = false;
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }

  FdHolder Target(::openat(Fd, Path.c_str(), O_RDONLY));
  if (unlikely(!Target.ok())) {
    return WasiUnexpect(fromErrNo(errno));
  }

  struct stat SysStat;
  if (NeedFile) {
    if (auto Res = ::fstat(Target.Fd, &SysStat); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  timespec Now;
  if (NeedNow) {
    if (auto Res = ::clock_gettime(CLOCK_REALTIME, &Now); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  timeval SysTimeval[2];
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    SysTimeval[0] = toTimeval(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    SysTimeval[0] = toTimeval(Now);
  } else {
    SysTimeval[0] = toTimeval(SysStat.st_atimespec);
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimeval[1] = toTimeval(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimeval[1] = toTimeval(Now);
  } else {
    SysTimeval[1] = toTimeval(SysStat.st_mtimespec);
  }

  if (auto Res = ::futimes(Target.Fd, SysTimeval); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::pathLink(const INode &Old, std::string OldPath,
                                 const INode &New,
                                 std::string NewPath) noexcept {
  if (auto Res = ::linkat(Old.Fd, OldPath.c_str(), New.Fd, NewPath.c_str(), 0);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<INode> INode::pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                                  __wasi_fdflags_t FdFlags,
                                  uint8_t VFSFlags) const noexcept {
  const int Flags = openFlags(OpenFlags, FdFlags, VFSFlags);

  if (auto NewFd = ::openat(Fd, Path.c_str(), Flags, 0644);
      unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    return INode(NewFd);
  }
}

WasiExpect<void> INode::pathReadlink(std::string Path, Span<char> Buffer,
                                     __wasi_size_t &NRead) const noexcept {
  if (auto Res = ::readlinkat(Fd, Path.c_str(), Buffer.data(), Buffer.size());
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NRead = Res;
  }

  return {};
}

WasiExpect<void> INode::pathRemoveDirectory(std::string Path) const noexcept {
  if (auto Res = ::unlinkat(Fd, Path.c_str(), AT_REMOVEDIR);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::pathRename(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept {
  if (auto Res = ::renameat(Old.Fd, OldPath.c_str(), New.Fd, NewPath.c_str());
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::pathSymlink(std::string OldPath,
                                    std::string NewPath) const noexcept {
  if (auto Res = ::symlinkat(OldPath.c_str(), Fd, NewPath.c_str());
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::pathUnlinkFile(std::string Path) const noexcept {
  if (auto Res = ::unlinkat(Fd, Path.c_str(), 0); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<Poller> INode::pollOneoff(__wasi_size_t NSubscriptions) noexcept {
  try {
    Poller P(NSubscriptions);
    if (unlikely(!P.ok())) {
      return WasiUnexpect(fromErrNo(errno));
    }
    return std::move(P);
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  int SysRiFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    SysRiFlags |= MSG_PEEK;
  }
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    SysRiFlags |= MSG_WAITALL;
  }

  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : RiData) {
    SysIOVs[SysIOVsSize].iov_base = IOV.data();
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysIOVs;
  SysMsgHdr.msg_iovlen = SysIOVsSize;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;
  SysMsgHdr.msg_flags = 0;

  /// Store recv bytes length and flags.
  if (auto Res = ::recvmsg(Fd, &SysMsgHdr, SysRiFlags); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NRead = Res;
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);
  if (SysMsgHdr.msg_flags & MSG_TRUNC) {
    RoFlags |= __WASI_ROFLAGS_RECV_DATA_TRUNCATED;
  }

  return {};
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t,
                                 __wasi_size_t &NWritten) const noexcept {
  int SysSiFlags = 0;

  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : SiData) {
    SysIOVs[SysIOVsSize].iov_base = const_cast<uint8_t *>(IOV.data());
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysIOVs;
  SysMsgHdr.msg_iovlen = SysIOVsSize;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;

  /// Store recv bytes length and flags.
  if (auto Res = ::sendmsg(Fd, &SysMsgHdr, SysSiFlags); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NWritten = Res;
  }

  return {};
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
  int SysFlags = 0;
  if (SdFlags == __WASI_SDFLAGS_RD) {
    SysFlags = SHUT_RD;
  } else if (SdFlags == __WASI_SDFLAGS_WR) {
    SysFlags = SHUT_WR;
  } else if (SdFlags == (__WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR)) {
    SysFlags = SHUT_RDWR;
  }

  if (auto Res = ::shutdown(Fd, SysFlags); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

__wasi_filetype_t INode::unsafeFiletype() const noexcept {
  return fromFileType(Stat->st_mode);
}

WasiExpect<__wasi_filetype_t> INode::filetype() const noexcept {
  if (!Stat) {
    if (auto Res = updateStat(); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
  }
  return unsafeFiletype();
}

bool INode::isDirectory() const noexcept {
  if (!Stat) {
    if (!updateStat()) {
      return false;
    }
  }
  return (Stat->st_mode & S_IFMT) == S_IFDIR;
}

bool INode::isSymlink() const noexcept {
  if (!Stat) {
    if (!updateStat()) {
      return false;
    }
  }
  return (Stat->st_mode & S_IFMT) == S_IFLNK;
}

WasiExpect<__wasi_filesize_t> INode::filesize() const noexcept {
  if (!Stat) {
    if (auto Res = updateStat(); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
  }
  return Stat->st_size;
}

bool INode::canBrowse() const noexcept {
  return ::faccessat(Fd, ".", X_OK, 0) == 0;
}

WasiExpect<void> INode::updateStat() const noexcept {
  Stat.emplace();
  if (unlikely(::fstat(Fd, &*Stat) != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

Poller::Poller(__wasi_size_t Count) : FdHolder(::kqueue()) {
  Events.reserve(Count);
}

WasiExpect<void> Poller::clock(__wasi_clockid_t, __wasi_timestamp_t Timeout,
                               __wasi_timestamp_t, __wasi_subclockflags_t Flags,
                               __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_CLOCK,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  int SysFlags = NOTE_NSECONDS;
  if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
    // TODO: Implement
  }

  struct kevent KEvent;
  EV_SET(&KEvent, 0, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT, SysFlags,
         Timeout, reinterpret_cast<void *>(Events.size() - 1));

  if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      unlikely(Ret < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> Poller::read(const INode &Node,
                              __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_FD_READ,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  struct kevent KEvent;
  EV_SET(&KEvent, Node.Fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0,
         reinterpret_cast<void *>(Events.size() - 1));

  if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      unlikely(Ret < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> Poller::write(const INode &Node,
                               __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_FD_WRITE,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  struct kevent KEvent;
  EV_SET(&KEvent, Node.Fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0,
         reinterpret_cast<void *>(Events.size() - 1));

  if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      unlikely(Ret < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> Poller::wait(CallbackType Callback) noexcept {
  std::vector<struct kevent> KEvents;
  try {
    KEvents.resize(Events.size());
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
  const auto Count =
      ::kevent(Fd, nullptr, 0, KEvents.data(), KEvents.size(), nullptr);
  if (unlikely(Count < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  for (int I = 0; I < Count; ++I) {
    auto &KEvent = KEvents[I];
    const auto Index = reinterpret_cast<size_t>(KEvent.udata);
    __wasi_filesize_t NBytes = 0;
    auto Flags = static_cast<__wasi_eventrwflags_t>(0);
    switch (Events[Index].type) {
    case __WASI_EVENTTYPE_CLOCK:
      break;
    case __WASI_EVENTTYPE_FD_READ: {
      if (KEvent.flags & EV_EOF) {
        Flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      int ReadBufUsed = 0;
      if (auto Res = ioctl(Fd, FIONREAD, &ReadBufUsed); unlikely(Res == 0)) {
        break;
      }
      NBytes = ReadBufUsed;
      break;
    }
    case __WASI_EVENTTYPE_FD_WRITE: {
      if (KEvent.flags & EV_EOF) {
        Flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      int WriteBufSize = 0;
      socklen_t IntSize = sizeof(WriteBufSize);
      if (auto Res =
              getsockopt(Fd, SOL_SOCKET, SO_SNDBUF, &WriteBufSize, &IntSize);
          unlikely(Res != 0)) {
        break;
      }
      int WriteBufUsed = 0;
      if (auto Res = ioctl(Fd, TIOCOUTQ, &WriteBufUsed); unlikely(Res != 0)) {
        break;
      }
      NBytes = WriteBufSize - WriteBufUsed;
      break;
    }
    }
    Callback(Events[Index].userdata, __WASI_ERRNO_SUCCESS, Events[Index].type,
             NBytes, Flags);
  }
  return {};
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
