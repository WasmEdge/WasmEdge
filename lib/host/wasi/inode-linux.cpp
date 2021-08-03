// SPDX-License-Identifier: Apache-2.0
#include "common/defines.h"
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#if WASMEDGE_OS_LINUX

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "linux.h"

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
  int Flags = O_NOFOLLOW;
#ifdef O_CLOEXEC
  Flags |= O_CLOEXEC;
#endif

  if (VFSFlags & VFS::Read) {
    if (VFSFlags & VFS::Write) {
      Flags |= O_RDWR;
    } else {
      Flags |= O_RDONLY;
    }
  } else if (VFSFlags & VFS::Write) {
    Flags |= O_WRONLY;
  } else {
#ifdef O_PATH
    Flags |= O_PATH;
#else
    Flags |= O_RDONLY;
#endif
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
  if ((FdFlags & __WASI_FDFLAGS_DSYNC) != 0) {
#ifdef O_DSYNC
    Flags |= O_DSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAGS_NONBLOCK) != 0) {
    Flags |= O_NONBLOCK;
  }
  if ((FdFlags & __WASI_FDFLAGS_RSYNC) != 0) {
#ifdef O_RSYNC
    Flags |= O_RSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAGS_SYNC) != 0) {
    Flags |= O_SYNC;
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

void TimerHolder::reset() noexcept {
  if (likely(Id.has_value())) {
    timer_delete(*Id);
    Id.reset();
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
    INode New(NewFd);
#ifndef O_CLOEXEC
    if (auto Res = ::fcntl(New.Fd, F_SETFD, FD_CLOEXEC); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
#endif
    return New;
  }
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t Offset,
                                 __wasi_filesize_t Len,
                                 __wasi_advice_t Advice) const noexcept {
  if (auto Res = ::posix_fadvise(Fd, Offset, Len, toAdvice(Advice));
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t Offset,
                                   __wasi_filesize_t Len) const noexcept {
  if (auto Res = ::posix_fallocate(Fd, Offset, Len); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept {
  if (auto Res = ::fdatasync(Fd); unlikely(Res != 0)) {
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
    SysFlag |= O_RSYNC;
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
  Filestat.atim = fromTimespec(Stat->st_atim);
  Filestat.mtim = fromTimespec(Stat->st_mtim);
  Filestat.ctim = fromTimespec(Stat->st_ctim);

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
#if __GLIBC_PREREQ(2, 6)
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
#else
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
    SysTimeval[0] = toTimeval(Stat->st_atim);
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimeval[1] = toTimeval(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimeval[1] = toTimeval(Now);
  } else {
    SysTimeval[1] = toTimeval(Stat->st_mtim);
  }

  if (auto Res = ::futimes(Fd, SysTimeval); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
#endif

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

#if __GLIBC_PREREQ(2, 10)
  /// Store read bytes length.
  if (auto Res = ::preadv(Fd, SysIOVs, SysIOVsSize, Offset);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NRead = Res;
  }
#else
  const auto OldOffset = ::lseek(Fd, 0, SEEK_CUR);
  if (OldOffset < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (::lseek(Fd, Offset, SEEK_SET) < 0 ||
      ::lseek(Fd, OldOffset, SEEK_SET) < 0) {
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
#endif

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

#if __GLIBC_PREREQ(2, 10)
  if (auto Res = ::pwritev(Fd, SysIOVs, SysIOVsSize, Offset);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NWritten = Res;
  }
#else
  const auto OldOffset = ::lseek(Fd, 0, SEEK_CUR);
  if (OldOffset < 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  if (::lseek(Fd, Offset, SEEK_SET) < 0 ||
      ::lseek(Fd, OldOffset, SEEK_SET) < 0) {
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
#endif

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
    Dir.Cookie = SysDirent->d_off;
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
  Filestat.atim = fromTimespec(SysFStat.st_atim);
  Filestat.mtim = fromTimespec(SysFStat.st_mtim);
  Filestat.ctim = fromTimespec(SysFStat.st_ctim);

  return {};
}

WasiExpect<void>
INode::pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                            __wasi_timestamp_t MTim,
                            __wasi_fstflags_t FstFlags) const noexcept {
#if __GLIBC_PREREQ(2, 6)
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
#else
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

#ifdef O_PATH
  const int OFlags = O_PATH;
#else
  const int OFlags = O_RDONLY;
#endif

  FdHolder Target(::openat(Fd, Path.c_str(), OFlags));
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
    SysTimeval[0] = toTimeval(SysStat.st_atim);
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimeval[1] = toTimeval(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimeval[1] = toTimeval(Now);
  } else {
    SysTimeval[1] = toTimeval(SysStat.st_mtim);
  }

  if (auto Res = ::futimes(Target.Fd, SysTimeval); unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
#endif

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
    INode New(NewFd);
#ifndef O_CLOEXEC
    if (auto Res = ::fcntl(New.Fd, F_SETFD, FD_CLOEXEC); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
#endif
    return New;
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
    return P;
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
}

WasiExpect<INode> INode::sockOpen(__wasi_address_family_t AddressFamily,
                                  __wasi_sock_type_t SockType) noexcept {

  int SysProtocol = IPPROTO_TCP;

  int SysDomain = 0;
  int SysType = 0;

  switch (AddressFamily) {
  case __WASI_ADDRESS_FAMILY_INET4:
    SysDomain = AF_INET;
    break;
  case __WASI_ADDRESS_FAMILY_INET6:
    SysDomain = AF_INET6;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  switch (SockType) {
  case __WASI_SOCK_TYPE_SOCK_DGRAM:
    SysType = SOCK_DGRAM;
    break;
  case __WASI_SOCK_TYPE_SOCK_STREAM:
    SysType = SOCK_STREAM;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  if (auto NewFd = ::socket(SysDomain, SysType, SysProtocol);
      unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    INode New(NewFd);
    return New;
  }
}

WasiExpect<void> INode::sockBind(uint8_t *Address, uint8_t AddressLength,
                                 uint16_t Port) noexcept {

  if (AddressLength == 4) {
    struct sockaddr_in ServerAddr;
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    std::memcpy(&ServerAddr.sin_addr.s_addr, Address, AddressLength);

    if (auto Res = ::bind(Fd, reinterpret_cast<struct sockaddr *>(&ServerAddr),
                          sizeof(ServerAddr));
        unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  } else if (AddressLength == 16) {
    struct sockaddr_in6 ServerAddr;

    ServerAddr.sin6_family = AF_INET6;
    ServerAddr.sin6_port = htons(Port);
    std::memcpy(ServerAddr.sin6_addr.s6_addr, Address, AddressLength);
    if (auto Res = ::bind(Fd, reinterpret_cast<struct sockaddr *>(&ServerAddr),
                          sizeof(ServerAddr));
        unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }
  return {};
}

WasiExpect<void> INode::sockListen(uint32_t Backlog) noexcept {
  if (auto Res = ::listen(Fd, Backlog); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<INode> INode::sockAccept(uint16_t Port) noexcept {
  struct sockaddr_in ServerSocketAddr;
  ServerSocketAddr.sin_family = AF_INET;
  ServerSocketAddr.sin_addr.s_addr = INADDR_ANY;
  ServerSocketAddr.sin_port = htons(Port);
  socklen_t AddressLen = sizeof(ServerSocketAddr);

  if (auto NewFd =
          ::accept(Fd, reinterpret_cast<struct sockaddr *>(&ServerSocketAddr),
                   &AddressLen);
      unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    INode New(NewFd);
    return New;
  }
}

WasiExpect<void> INode::sockConnect(uint8_t *Address, uint8_t AddressLength,
                                    uint16_t Port) noexcept {
  if (AddressLength == 4) {
    struct sockaddr_in ClientSocketAddr;
    ClientSocketAddr.sin_family = AF_INET;
    ClientSocketAddr.sin_port = htons(Port);
    std::memcpy(&ClientSocketAddr.sin_addr.s_addr, Address, AddressLength);

    if (auto Res = ::connect(
            Fd, reinterpret_cast<struct sockaddr *>(&ClientSocketAddr),
            sizeof(ClientSocketAddr));
        unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  } else if (AddressLength == 16) {
    struct sockaddr_in6 ClientSocketAddr;

    ClientSocketAddr.sin6_family = AF_INET6;
    ClientSocketAddr.sin6_port = htons(Port);
    std::memcpy(ClientSocketAddr.sin6_addr.s6_addr, Address, AddressLength);
    if (auto Res =
            ::bind(Fd, reinterpret_cast<struct sockaddr *>(&ClientSocketAddr),
                   sizeof(ClientSocketAddr));
        unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }
  return {};
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

#if __GLIBC_PREREQ(2, 8)
WasiExpect<void> Poller::Timer::create(__wasi_clockid_t Clock,
                                       __wasi_timestamp_t Timeout,
                                       __wasi_timestamp_t,
                                       __wasi_subclockflags_t Flags) noexcept {
  Fd = timerfd_create(toClockId(Clock), TFD_NONBLOCK | TFD_CLOEXEC);
  if (unlikely(Fd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  int SysFlags = 0;
  if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
    SysFlags |= TFD_TIMER_ABSTIME;
  }
  itimerspec Spec{toTimespec(0), toTimespec(Timeout)};
  if (auto Res = timerfd_settime(Fd, SysFlags, &Spec, nullptr);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}
#else
namespace {
static void sigevCallback(union sigval Value) noexcept {
  const uint64_t One = 1;
  ::write(Value.sival_int, &One, sizeof(One));
}
} // namespace

WasiExpect<void> Poller::Timer::create(__wasi_clockid_t Clock,
                                       __wasi_timestamp_t Timeout,
                                       __wasi_timestamp_t,
                                       __wasi_subclockflags_t Flags) noexcept {
  FdHolder Timer, Notify;
  {
    int PipeFd[2] = {-1, -1};

    if (auto Res = ::pipe(PipeFd); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
    Timer.emplace(PipeFd[0]);
    Notify.emplace(PipeFd[1]);
  }

  timer_t TId;
  {
    sigevent Event;
    Event.sigev_notify = SIGEV_THREAD;
    Event.sigev_notify_function = &sigevCallback;
    Event.sigev_value.sival_int = Notify.Fd;
    Event.sigev_notify_attributes = nullptr;

    if (unlikely(::fcntl(Timer.Fd, F_SETFD, FD_CLOEXEC) != 0 ||
                 ::fcntl(Notify.Fd, F_SETFD, FD_CLOEXEC) != 0 ||
                 ::timer_create(toClockId(Clock), &Event, &TId) < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  TimerHolder TimerId(TId);
  {
    int SysFlags = 0;
    if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
      SysFlags |= TIMER_ABSTIME;
    }
    itimerspec Spec{toTimespec(0), toTimespec(Timeout)};
    if (auto Res = ::timer_settime(TId, SysFlags, &Spec, nullptr);
        unlikely(Res < 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  this->FdHolder::operator=(std::move(Timer));
  this->Notify = std::move(Notify);
  this->TimerId = std::move(TimerId);
  return {};
}
#endif

Poller::Poller(__wasi_size_t Count)
    : FdHolder(
#if __GLIBC_PREREQ(2, 9)
          ::epoll_create1(EPOLL_CLOEXEC)
#else
          ::epoll_create(Count)
#endif
      ) {
#if !__GLIBC_PREREQ(2, 9)
  if (auto Res = ::fcntl(Fd, F_SETFD, FD_CLOEXEC); unlikely(Res != 0)) {
    reset();
    return;
  }
#endif
  Events.reserve(Count);
}

WasiExpect<void> Poller::clock(__wasi_clockid_t Clock,
                               __wasi_timestamp_t Timeout,
                               __wasi_timestamp_t Precision,
                               __wasi_subclockflags_t Flags,
                               __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_CLOCK,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
    Timers.emplace_back();
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  auto &Timer = Timers.back();
  if (auto Res = Timer.create(Clock, Timeout, Precision, Flags);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  epoll_event EPollEvent;
  EPollEvent.events = EPOLLIN;
#if defined(EPOLLRDHUP)
  EPollEvent.events |= EPOLLRDHUP;
#endif
  EPollEvent.data.u64 = Events.size() - 1;

  if (auto Res = ::epoll_ctl(this->Fd, EPOLL_CTL_ADD, Timer.Fd, &EPollEvent);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> Poller::read(const INode &Fd,
                              __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_FD_READ,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  epoll_event EPollEvent;
  EPollEvent.events = EPOLLIN;
#if defined(EPOLLRDHUP)
  EPollEvent.events |= EPOLLRDHUP;
#endif
  EPollEvent.data.u64 = Events.size() - 1;

  if (auto Res = ::epoll_ctl(this->Fd, EPOLL_CTL_ADD, Fd.Fd, &EPollEvent);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> Poller::write(const INode &Fd,
                               __wasi_userdata_t UserData) noexcept {
  try {
    Events.push_back({UserData,
                      __WASI_ERRNO_SUCCESS,
                      __WASI_EVENTTYPE_FD_WRITE,
                      {0, static_cast<__wasi_eventrwflags_t>(0)}});
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
  epoll_event EPollEvent;
  EPollEvent.events = EPOLLOUT;
#if defined(EPOLLRDHUP)
  EPollEvent.events |= EPOLLRDHUP;
#endif
  EPollEvent.data.u64 = Events.size() - 1;

  if (auto Res = ::epoll_ctl(this->Fd, EPOLL_CTL_ADD, Fd.Fd, &EPollEvent);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> Poller::wait(CallbackType Callback) noexcept {
  std::vector<struct epoll_event> EPollEvents;
  try {
    EPollEvents.resize(Events.size());
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
  const int Count =
      ::epoll_wait(Fd, EPollEvents.data(), EPollEvents.size(), -1);
  if (unlikely(Count < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  for (int I = 0; I < Count; ++I) {
    auto &EPollEvent = EPollEvents[I];
    const auto Index = EPollEvent.data.u64;
    __wasi_filesize_t NBytes = 0;
    auto Flags = static_cast<__wasi_eventrwflags_t>(0);
    switch (Events[Index].type) {
    case __WASI_EVENTTYPE_CLOCK:
      break;
    case __WASI_EVENTTYPE_FD_READ: {
      if (EPollEvent.events & EPOLLHUP) {
        Flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      int ReadBufUsed = 0;
      if (auto Res = ::ioctl(Fd, FIONREAD, &ReadBufUsed); unlikely(Res == 0)) {
        break;
      }
      NBytes = ReadBufUsed;
      break;
    }
    case __WASI_EVENTTYPE_FD_WRITE: {
      if (EPollEvent.events & EPOLLHUP) {
        Flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      int WriteBufSize = 0;
      socklen_t IntSize = sizeof(WriteBufSize);
      if (auto Res =
              ::getsockopt(Fd, SOL_SOCKET, SO_SNDBUF, &WriteBufSize, &IntSize);
          unlikely(Res != 0)) {
        break;
      }
      int WriteBufUsed = 0;
      if (auto Res = ::ioctl(Fd, TIOCOUTQ, &WriteBufUsed); unlikely(Res != 0)) {
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
