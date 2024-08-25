// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_MACOS

#include "common/errcode.h"
#include "common/spdlog.h"
#include "common/variant.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "macos.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <vector>

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

inline constexpr __wasi_size_t
calculateAddrinfoLinkedListSize(struct addrinfo *const Addrinfo) {
  __wasi_size_t Length = 0;
  for (struct addrinfo *TmpPointer = Addrinfo; TmpPointer != nullptr;
       TmpPointer = TmpPointer->ai_next) {
    Length++;
  }
  return Length;
};

constexpr int openFlags(__wasi_oflags_t OpenFlags, __wasi_fdflags_t FdFlags,
                        VFS::Flags VFSFlags) noexcept {
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
  if ((OpenFlags & __WASI_OFLAGS_TRUNC) && (VFSFlags & VFS::Write)) {
    Flags |= O_TRUNC;
  }

  // Convert file descriptor flags.
  if ((FdFlags & (__WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC |
                  __WASI_FDFLAGS_SYNC)) != 0) {
    Flags |= O_SYNC;
  }
  if ((FdFlags & __WASI_FDFLAGS_NONBLOCK) != 0) {
    Flags |= O_NONBLOCK;
  }

  return Flags;
}

std::pair<const char *, std::unique_ptr<char[]>>
createNullTerminatedString(std::string_view View) noexcept {
  const char *CStr = nullptr;
  std::unique_ptr<char[]> Buffer;
  if (!View.empty()) {
    if (const auto Pos = View.find_first_of('\0');
        Pos != std::string_view::npos) {
      CStr = View.data();
    } else {
      Buffer = std::make_unique<char[]>(View.size() + 1);
      std::copy(View.begin(), View.end(), Buffer.get());
      CStr = Buffer.get();
    }
  }
  return {CStr, std::move(Buffer)};
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
                              VFS::Flags VFSFlags) noexcept {
  const int Flags = openFlags(OpenFlags, FdFlags, VFSFlags);

  if (auto NewFd = ::open(Path.c_str(), Flags, 0644); unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    return INode(NewFd, true, FdFlags & __WASI_FDFLAGS_APPEND);
  }
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t, __wasi_filesize_t,
                                 __wasi_advice_t) const noexcept {
  // Not supported, just ignore it.
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
    // File is already large enough.
    return {};
  }

  // Try to allocate contiguous space.
  fstore_t Store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0,
                    static_cast<int64_t>(Len), 0};
  if (auto Res = ::fcntl(Fd, F_PREALLOCATE, &Store); unlikely(Res < 0)) {
    // Try to allocate sparse space.
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
    if (Append) {
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

  Append = FdFlags & __WASI_FDFLAGS_APPEND;
  return {};
}

WasiExpect<void>
INode::fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept {
  if (auto Res = updateStat(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  // Zeroing out these values to prevent leaking information about the host
  // environment from special fd such as stdin, stdout and stderr.
  Filestat.dev = isSpecialFd(Fd) ? 0 : Stat->st_dev;
  Filestat.ino = isSpecialFd(Fd) ? 0 : Stat->st_ino;
  Filestat.filetype = unsafeFiletype();
  Filestat.nlink = isSpecialFd(Fd) ? 0 : Stat->st_nlink;
  Filestat.size = isSpecialFd(Fd) ? 0 : Stat->st_size;
  Filestat.atim = isSpecialFd(Fd) ? 0 : fromTimespec(Stat->st_atimespec);
  Filestat.mtim = isSpecialFd(Fd) ? 0 : fromTimespec(Stat->st_mtimespec);
  Filestat.ctim = isSpecialFd(Fd) ? 0 : fromTimespec(Stat->st_ctimespec);

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

// Due to the unfortunate design of wasi::fd_readdir, It's nearly impossible to
// provide a correct implementation. The below implementation is just a
// workaround for most usages and may not be correct in some edge cases. The
// readdir entry API is going to be updated to use a stream type, so we don't
// have to deal with it right now.
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

  if (Cookie == 0) {
    ::rewinddir(Dir.Dir);
  } else if (unlikely(Cookie != Dir.Cookie)) {
    ::seekdir(Dir.Dir, Cookie);
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
      Dir.Buffer.clear();
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
    Dir.Cookie = ::telldir(Dir.Dir);
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

  if (Append) {
    ::lseek(Fd, 0, SEEK_END);
  }

  if (auto Res = ::writev(Fd, SysIOVs, SysIOVsSize); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NWritten = Res;
  }

  return {};
}

WasiExpect<uint64_t> INode::getNativeHandler() const noexcept {
  return static_cast<uint64_t>(Fd);
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

    if (auto Res =
            ::utimensat(Fd, Path.c_str(), SysTimespec, AT_SYMLINK_NOFOLLOW);
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

  FdHolder Target(::openat(Fd, Path.c_str(), O_RDONLY | O_SYMLINK));
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
                                  VFS::Flags VFSFlags) const noexcept {
  const int Flags = openFlags(OpenFlags, FdFlags, VFSFlags);

  if (auto NewFd = ::openat(Fd, Path.c_str(), Flags, 0644);
      unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    return INode(NewFd, true, FdFlags & __WASI_FDFLAGS_APPEND);
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

WasiExpect<void> INode::getAddrinfo(std::string_view Node,
                                    std::string_view Service,
                                    const __wasi_addrinfo_t &Hint,
                                    uint32_t MaxResLength,
                                    Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                                    Span<__wasi_sockaddr_t *> WasiSockaddrArray,
                                    Span<char *> AiAddrSaDataArray,
                                    Span<char *> AiCanonnameArray,
                                    /*Out*/ __wasi_size_t &ResLength) noexcept {
  const auto [NodeCStr, NodeBuf] = createNullTerminatedString(Node);
  const auto [ServiceCStr, ServiceBuf] = createNullTerminatedString(Service);

  struct addrinfo SysHint;
  SysHint.ai_flags = toAIFlags(Hint.ai_flags);
  SysHint.ai_family = toAddressFamily(Hint.ai_family);
  SysHint.ai_socktype = toSockType(Hint.ai_socktype);
  SysHint.ai_protocol = toProtocol(Hint.ai_protocol);
  SysHint.ai_addrlen = Hint.ai_addrlen;
  SysHint.ai_addr = nullptr;
  SysHint.ai_canonname = nullptr;
  SysHint.ai_next = nullptr;

  struct addrinfo *SysResPtr = nullptr;
  if (auto Res = ::getaddrinfo(NodeCStr, ServiceCStr, &SysHint, &SysResPtr);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromEAIErrNo(Res));
  }
  // calculate ResLength
  if (ResLength = calculateAddrinfoLinkedListSize(SysResPtr);
      ResLength > MaxResLength) {
    ResLength = MaxResLength;
  }

  struct addrinfo *SysResItem = SysResPtr;
  for (uint32_t Idx = 0; Idx < ResLength; Idx++) {
    auto &CurAddrinfo = WasiAddrinfoArray[Idx];
    CurAddrinfo->ai_flags = fromAIFlags(SysResItem->ai_flags);
    CurAddrinfo->ai_socktype = fromSockType(SysResItem->ai_socktype);
    CurAddrinfo->ai_protocol = fromProtocol(SysResItem->ai_protocol);
    CurAddrinfo->ai_family = fromAddressFamily(SysResItem->ai_family);
    CurAddrinfo->ai_addrlen = SysResItem->ai_addrlen;

    // process ai_canonname in addrinfo
    if (SysResItem->ai_canonname != nullptr) {
      CurAddrinfo->ai_canonname_len = std::strlen(SysResItem->ai_canonname);
      auto &CurAiCanonname = AiCanonnameArray[Idx];
      std::memcpy(CurAiCanonname, SysResItem->ai_canonname,
                  CurAddrinfo->ai_canonname_len + 1);
    } else {
      CurAddrinfo->ai_canonname_len = 0;
    }

    // process socket address
    if (SysResItem->ai_addrlen > 0) {
      auto &CurSockaddr = WasiSockaddrArray[Idx];
      CurSockaddr->sa_family =
          fromAddressFamily(SysResItem->ai_addr->sa_family);

      // process sa_data in socket address
      size_t SaSize = 0;
      switch (CurSockaddr->sa_family) {
      case __WASI_ADDRESS_FAMILY_INET4:
        SaSize = sizeof(sockaddr_in) - offsetof(sockaddr_in, sin_port);
        break;
      case __WASI_ADDRESS_FAMILY_INET6:
        SaSize = sizeof(sockaddr_in6) - offsetof(sockaddr_in6, sin6_port);
        break;
      default:
        assumingUnreachable();
      }
      std::memcpy(AiAddrSaDataArray[Idx], SysResItem->ai_addr->sa_data, SaSize);
      CurSockaddr->sa_data_len = __wasi_size_t(SaSize);
    }
    // process ai_next in addrinfo
    SysResItem = SysResItem->ai_next;
  }
  ::freeaddrinfo(SysResPtr);

  return {};
}

WasiExpect<INode> INode::sockOpen(__wasi_address_family_t AddressFamily,
                                  __wasi_sock_type_t SockType) noexcept {
  int SysProtocol = IPPROTO_IP;
  int SysDomain = 0;
  int SysType = 0;

  switch (AddressFamily) {
  case __WASI_ADDRESS_FAMILY_INET4:
    SysDomain = AF_INET;
    break;
  case __WASI_ADDRESS_FAMILY_INET6:
    SysDomain = AF_INET6;
    break;
  case __WASI_ADDRESS_FAMILY_AF_UNIX:
    SysDomain = AF_UNIX;
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

struct SockEmptyAddr {};
using VarAddrT = std::variant<SockEmptyAddr, sockaddr_storage, sockaddr,
                              sockaddr_in, sockaddr_in6, sockaddr_un>;

struct VarAddrBuf {
  template <typename T> sockaddr *operator()(T &V) {
    return reinterpret_cast<struct sockaddr *>(&V);
  }
  sockaddr *operator()(SockEmptyAddr &) { return nullptr; }
};

struct VarAddrSize {
  template <typename T> int operator()(const T &) { return sizeof(T); }
  int operator()(const SockEmptyAddr &) { return 0; }
};

static VarAddrT sockAddressAssignHelper(__wasi_address_family_t AddrFamily,
                                        const Span<const uint8_t> &Address,
                                        uint16_t Port) {
  VarAddrT Addr;
  if (Address.size() == 0) {
    Addr.emplace<SockEmptyAddr>();
  } else if (AddrFamily == __WASI_ADDRESS_FAMILY_INET4) {
    auto &ServerAddr4 = Addr.emplace<sockaddr_in>();

    ServerAddr4.sin_family = AF_INET;
    ServerAddr4.sin_port = htons(Port);
    assuming(Address.size() >= sizeof(in_addr));
    std::memcpy(&ServerAddr4.sin_addr, Address.data(), sizeof(in_addr));
  } else if (AddrFamily == __WASI_ADDRESS_FAMILY_INET6) {
    auto &ServerAddr6 = Addr.emplace<sockaddr_in6>();

    ServerAddr6.sin6_family = AF_INET6;
    ServerAddr6.sin6_port = htons(Port);
    ServerAddr6.sin6_flowinfo = 0;
    assuming(Address.size() >= sizeof(in6_addr));
    std::memcpy(&ServerAddr6.sin6_addr, Address.data(), sizeof(in6_addr));
  } else if (AddrFamily == __WASI_ADDRESS_FAMILY_AF_UNIX) {
    auto &ServerAddrUN = Addr.emplace<sockaddr_un>();

    ServerAddrUN.sun_family = AF_UNIX;
    // The length of sockaddr_un::sun_path is depend on cruuent system
    // We should always check the size of it.
    assuming(Address.size() >= sizeof(sockaddr_un::sun_path));
    std::memcpy(&ServerAddrUN.sun_path, Address.data(),
                sizeof(sockaddr_un::sun_path));
  } else {
    assumingUnreachable();
  }

  return Addr;
}

WasiExpect<void> INode::sockBind(__wasi_address_family_t AddressFamily,
                                 Span<const uint8_t> Address,
                                 uint16_t Port) noexcept {
  auto AddressBuffer = sockAddressAssignHelper(AddressFamily, Address, Port);

  auto ServerAddr = std::visit(VarAddrBuf(), AddressBuffer);
  int Size = std::visit(VarAddrSize(), AddressBuffer);

  if (auto Res = ::bind(Fd, ServerAddr, Size); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> INode::sockListen(int32_t Backlog) noexcept {
  if (auto Res = ::listen(Fd, Backlog); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<INode> INode::sockAccept(__wasi_fdflags_t FdFlags) noexcept {
  int NewFd;
  if (NewFd = ::accept(Fd, nullptr, nullptr); unlikely(NewFd < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  INode New(NewFd);

  if (FdFlags & __WASI_FDFLAGS_NONBLOCK) {
    int SysFlag = fcntl(NewFd, F_GETFL, 0);
    SysFlag |= O_NONBLOCK;
    if (auto Res = ::fcntl(Fd, F_SETFL, SysFlag); unlikely(Res != 0)) {
      return WasiUnexpect(fromErrNo(errno));
    }
  }

  return New;
}

WasiExpect<void> INode::sockConnect(__wasi_address_family_t AddressFamily,
                                    Span<const uint8_t> Address,
                                    uint16_t Port) noexcept {
  auto AddressBuffer = sockAddressAssignHelper(AddressFamily, Address, Port);

  auto ClientAddr = std::visit(VarAddrBuf(), AddressBuffer);
  int Size = std::visit(VarAddrSize(), AddressBuffer);

  if (auto Res = ::connect(Fd, ClientAddr, Size); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  return sockRecvFrom(RiData, RiFlags, nullptr, {}, nullptr, NRead, RoFlags);
}

WasiExpect<void> INode::sockRecvFrom(Span<Span<uint8_t>> RiData,
                                     __wasi_riflags_t RiFlags,
                                     __wasi_address_family_t *AddressFamilyPtr,
                                     Span<uint8_t> Address, uint16_t *PortPtr,
                                     __wasi_size_t &NRead,
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

  const bool NeedAddress =
      AddressFamilyPtr != nullptr || !Address.empty() || PortPtr != nullptr;
  sockaddr_storage SockAddr = {};
  msghdr SysMsgHdr;
  if (NeedAddress) {
    SysMsgHdr.msg_name = &SockAddr;
    SysMsgHdr.msg_namelen = sizeof(SockAddr);
  } else {
    SysMsgHdr.msg_name = nullptr;
    SysMsgHdr.msg_namelen = 0;
  }
  SysMsgHdr.msg_iov = SysIOVs;
  SysMsgHdr.msg_iovlen = SysIOVsSize;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;
  SysMsgHdr.msg_flags = 0;

  // Store recv bytes length and flags.
  if (auto Res = ::recvmsg(Fd, &SysMsgHdr, SysRiFlags); unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  } else {
    NRead = Res;
  }

  if (NeedAddress) {
    switch (SockAddr.ss_family) {
    case AF_UNSPEC: {
      spdlog::warn("remote address unavailable");
      // if ss_family is AF_UNSPEC, the access of the other members are
      // undefined.
      break;
    }
    case AF_INET: {
      const auto &SockAddr4 = reinterpret_cast<sockaddr_in &>(SockAddr);
      if (AddressFamilyPtr) {
        *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
      }
      if (Address.size() >= sizeof(in_addr)) {
        std::memcpy(Address.data(), &SockAddr4.sin_addr, sizeof(in_addr));
      }
      if (PortPtr != nullptr) {
        *PortPtr = SockAddr4.sin_port;
      }
      break;
    }
    case AF_INET6: {
      const auto &SockAddr6 = reinterpret_cast<sockaddr_in6 &>(SockAddr);
      if (AddressFamilyPtr) {
        *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
      }
      if (Address.size() >= sizeof(in6_addr)) {
        std::memcpy(Address.data(), &SockAddr6.sin6_addr, sizeof(in6_addr));
      }
      if (PortPtr != nullptr) {
        *PortPtr = SockAddr6.sin6_port;
      }
      break;
    }
    case AF_UNIX: {
      const auto &SockAddrUN = reinterpret_cast<sockaddr_un &>(SockAddr);
      if (AddressFamilyPtr) {
        *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_AF_UNIX;
      }
      if (Address.size() >= sizeof(sockaddr_un::sun_path)) {
        std::memcpy(Address.data(), &SockAddrUN.sun_path,
                    sizeof(sockaddr_un::sun_path));
      } else {
        return WasiUnexpect(__WASI_ERRNO_INVAL);
      }
      break;
    }
    default:
      return WasiUnexpect(__WASI_ERRNO_NOSYS);
    }
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);
  if (SysMsgHdr.msg_flags & MSG_TRUNC) {
    RoFlags |= __WASI_ROFLAGS_RECV_DATA_TRUNCATED;
  }

  return {};
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t SiFlags,
                                 __wasi_size_t &NWritten) const noexcept {
  return sockSendTo(SiData, SiFlags, __WASI_ADDRESS_FAMILY_UNSPEC, {}, 0,
                    NWritten);
}

WasiExpect<void> INode::sockSendTo(Span<Span<const uint8_t>> SiData,
                                   __wasi_siflags_t,
                                   __wasi_address_family_t AddressFamily,
                                   Span<const uint8_t> Address, uint16_t Port,
                                   __wasi_size_t &NWritten) const noexcept {
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
  int SysSiFlags = MSG_NOSIGNAL;
  sockaddr *ClientAddr = nullptr;
  socklen_t MsgNameLen = 0;
  VarAddrT AddressBuffer;

  if (Address.size()) {
    AddressBuffer = sockAddressAssignHelper(AddressFamily, Address, Port);
    ClientAddr = std::visit(VarAddrBuf(), AddressBuffer);
    MsgNameLen = std::visit(VarAddrSize(), AddressBuffer);
  }

  iovec SysIOVs[kIOVMax];
  size_t SysIOVsSize = 0;
  for (auto &IOV : SiData) {
    SysIOVs[SysIOVsSize].iov_base = const_cast<uint8_t *>(IOV.data());
    SysIOVs[SysIOVsSize].iov_len = IOV.size();
    ++SysIOVsSize;
  }

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = MsgNameLen == 0 ? nullptr : ClientAddr;
  SysMsgHdr.msg_namelen = MsgNameLen;
  SysMsgHdr.msg_iov = SysIOVs;
  SysMsgHdr.msg_iovlen = SysIOVsSize;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;

  // Store recv bytes length and flags.
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

WasiExpect<void> INode::sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   Span<uint8_t> &Flag) const noexcept {
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  socklen_t Size = static_cast<socklen_t>(Flag.size());
  if (auto Res =
          ::getsockopt(Fd, SysSockOptLevel, SysSockOptName, Flag.data(), &Size);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  switch (SockOptName) {
  case __WASI_SOCK_OPT_SO_ERROR: {
    assuming(Size == sizeof(int32_t));
    Flag = Flag.first(static_cast<size_t>(Size));
    auto *Error = reinterpret_cast<int32_t *>(Flag.data());
    *Error = static_cast<int32_t>(fromErrNo(*Error));
    break;
  }
  case __WASI_SOCK_OPT_SO_TYPE: {
    assuming(Size == sizeof(int32_t));
    Flag = Flag.first(static_cast<size_t>(Size));
    auto &SockType = *reinterpret_cast<int32_t *>(Flag.data());
    SockType = static_cast<int32_t>(fromSockType(SockType));
    break;
  }
  default:
    Flag = Flag.first(static_cast<size_t>(Size));
  }

  return {};
}

WasiExpect<void> INode::sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   Span<const uint8_t> Flag) const noexcept {
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);

  if (auto Res = ::setsockopt(Fd, SysSockOptLevel, SysSockOptName, Flag.data(),
                              Flag.size());
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  return {};
}

WasiExpect<void>
INode::sockGetLocalAddr(__wasi_address_family_t *AddressFamilyPtr,
                        Span<uint8_t> Address,
                        uint16_t *PortPtr) const noexcept {
  sockaddr_storage SocketAddr = {};
  socklen_t Slen = sizeof(SocketAddr);

  if (auto Res =
          ::getsockname(Fd, reinterpret_cast<sockaddr *>(&SocketAddr), &Slen);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  switch (SocketAddr.ss_family) {
  case AF_INET: {
    if (Address.size() < sizeof(in_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr4 = reinterpret_cast<sockaddr_in &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr4.sin_port);
    }
    std::memcpy(Address.data(), &SocketAddr4.sin_addr, sizeof(in_addr));
    return {};
  }
  case AF_INET6: {
    if (Address.size() < sizeof(in6_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr6 = reinterpret_cast<sockaddr_in6 &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr6.sin6_port);
    }
    std::memcpy(Address.data(), &SocketAddr6.sin6_addr, sizeof(in6_addr));
    return {};
  }
  case AF_UNIX: {
    if (Address.size() < sizeof(sockaddr_un::sun_path)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddrUN = reinterpret_cast<sockaddr_un &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_AF_UNIX;
    }

    std::memcpy(Address.data(), &SocketAddrUN.sun_path,
                sizeof(sockaddr_un::sun_path));
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

WasiExpect<void>
INode::sockGetPeerAddr(__wasi_address_family_t *AddressFamilyPtr,
                       Span<uint8_t> Address,
                       uint16_t *PortPtr) const noexcept {
  sockaddr_storage SocketAddr = {};
  socklen_t Slen = sizeof(SocketAddr);

  if (auto Res =
          ::getpeername(Fd, reinterpret_cast<sockaddr *>(&SocketAddr), &Slen);
      unlikely(Res < 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  switch (SocketAddr.ss_family) {
  case AF_INET: {
    if (Address.size() < sizeof(in_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr4 = reinterpret_cast<sockaddr_in &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr4.sin_port);
    }
    std::memcpy(Address.data(), &SocketAddr4.sin_addr, sizeof(in_addr));
    return {};
  }
  case AF_INET6: {
    if (Address.size() < sizeof(in6_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr6 = reinterpret_cast<sockaddr_in6 &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr6.sin6_port);
    }
    std::memcpy(Address.data(), &SocketAddr6.sin6_addr, sizeof(in6_addr));
    return {};
  }
  case AF_UNIX: {
    if (Address.size() < sizeof(sockaddr_un::sun_path)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddrUN = reinterpret_cast<sockaddr_un &>(SocketAddr);
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_AF_UNIX;
    }

    std::memcpy(Address.data(), &SocketAddrUN.sun_path,
                sizeof(sockaddr_un::sun_path));
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
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

Poller::Poller(PollerContext &C) noexcept : FdHolder(::kqueue()), Ctx(C) {}

WasiExpect<void> Poller::prepare(Span<__wasi_event_t> E) noexcept {
  WasiEvents = E;
  try {
    Events.reserve(E.size());
    KEvents.reserve(Events.size());
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }

  return {};
}

void Poller::clock(__wasi_clockid_t, __wasi_timestamp_t Timeout,
                   __wasi_timestamp_t, __wasi_subclockflags_t Flags,
                   __wasi_userdata_t UserData) noexcept {
  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_CLOCK;

  const uint64_t Ident = NextTimerId++;

  uint32_t FFlags = NOTE_NSECONDS;
  if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
#ifdef NOTE_ABSOLUTE
    FFlags |= NOTE_ABSOLUTE;
#else
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOSYS;
    return;
#endif
  }

  struct kevent KEvent;
  EV_SET(&KEvent, Ident, EVFILT_TIMER, EV_ADD | EV_ENABLE, FFlags, Timeout,
         &Event);

  if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      unlikely(Ret < 0)) {
    Event.Valid = true;
    Event.error = fromErrNo(errno);
    return;
  }
}

void Poller::close(const INode &) noexcept {}

void Poller::read(const INode &Node, TriggerType Trigger,
                  __wasi_userdata_t UserData) noexcept {
  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_FD_READ;

  assuming(Node.Fd != Fd);
  try {
    auto [Iter, Added] = FdDatas.try_emplace(Node.Fd);

    if (unlikely(!Added && Iter->second.ReadEvent != nullptr)) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_EXIST;
      return;
    }
    Iter->second.ReadEvent = &Event;

    uint16_t Flags = EV_ADD | EV_ENABLE;
    if (Trigger == TriggerType::Edge) {
      Flags |= EV_CLEAR;
    }

    struct kevent KEvent;
    EV_SET(&KEvent, Node.Fd, EVFILT_READ, Flags, 0, 0, &Event);

    if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
        unlikely(Ret < 0)) {
      if (Added) {
        FdDatas.erase(Iter);
      } else {
        Iter->second.ReadEvent = nullptr;
      }
      Event.Valid = true;
      Event.error = fromErrNo(errno);
      return;
    }
  } catch (std::bad_alloc &) {
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOMEM;
    return;
  }
}

void Poller::write(const INode &Node, TriggerType Trigger,
                   __wasi_userdata_t UserData) noexcept {
  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_FD_WRITE;

  assuming(Node.Fd != Fd);
  try {
    auto [Iter, Added] = FdDatas.try_emplace(Node.Fd);

    if (unlikely(!Added && Iter->second.WriteEvent != nullptr)) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_EXIST;
      return;
    }
    Iter->second.WriteEvent = &Event;

    uint16_t Flags = EV_ADD | EV_ENABLE;
    if (Trigger == TriggerType::Edge) {
      Flags |= EV_CLEAR;
    }

    struct kevent KEvent;
    EV_SET(&KEvent, Node.Fd, EVFILT_WRITE, Flags, 0, 0, &Event);

    if (const auto Ret = ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
        unlikely(Ret < 0)) {
      if (Added) {
        FdDatas.erase(Iter);
      } else {
        Iter->second.WriteEvent = nullptr;
      }
      Event.Valid = true;
      Event.error = fromErrNo(errno);
      return;
    }
  } catch (std::bad_alloc &) {
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOMEM;
    return;
  }
}

void Poller::wait() noexcept {
  for (const auto &[NodeFd, FdData] : OldFdDatas) {
    if (auto Iter = FdDatas.find(NodeFd); Iter == FdDatas.end()) {
      // Remove unused event, ignore failed.
      if (FdData.ReadEvent) {
        struct kevent KEvent;
        EV_SET(&KEvent, NodeFd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      }
      if (FdData.WriteEvent) {
        struct kevent KEvent;
        EV_SET(&KEvent, NodeFd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
      }
    }
  }

  KEvents.resize(Events.size());
  const int Count =
      ::kevent(Fd, nullptr, 0, KEvents.data(), KEvents.size(), nullptr);
  if (unlikely(Count < 0)) {
    const auto Error = fromErrNo(errno);
    for (auto &Event : Events) {
      Event.Valid = true;
      Event.error = Error;
    }
    return;
  }

  for (int I = 0; I < Count; ++I) {
    auto &KEvent = KEvents[I];
    auto &Event = *reinterpret_cast<OptionalEvent *>(KEvent.udata);
    Event.Valid = true;
    Event.error = __WASI_ERRNO_SUCCESS;
    switch (Event.type) {
    case __WASI_EVENTTYPE_CLOCK:
      break;
    case __WASI_EVENTTYPE_FD_READ: {
      Event.fd_readwrite.flags = static_cast<__wasi_eventrwflags_t>(0);
      if (KEvent.flags & EV_EOF) {
        Event.fd_readwrite.flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      bool UnknownNBytes = false;
      int ReadBufUsed = 0;
      if (auto Res = ::ioctl(KEvent.ident, FIONREAD, &ReadBufUsed);
          unlikely(Res == 0)) {
        UnknownNBytes = true;
      }
      if (UnknownNBytes) {
        Event.fd_readwrite.nbytes = 1;
      } else {
        Event.fd_readwrite.nbytes = ReadBufUsed;
      }
      break;
    }
    case __WASI_EVENTTYPE_FD_WRITE: {
      Event.fd_readwrite.flags = static_cast<__wasi_eventrwflags_t>(0);
      if (KEvent.flags & EV_EOF) {
        Event.fd_readwrite.flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }
      bool UnknownNBytes = false;
      int WriteBufSize = 0;
      socklen_t IntSize = sizeof(WriteBufSize);
      if (auto Res = ::getsockopt(KEvent.ident, SOL_SOCKET, SO_SNDBUF,
                                  &WriteBufSize, &IntSize);
          unlikely(Res != 0)) {
        UnknownNBytes = true;
      }
      int WriteBufUsed = 0;
      if (auto Res = ::ioctl(KEvent.ident, TIOCOUTQ, &WriteBufUsed);
          unlikely(Res != 0)) {
        UnknownNBytes = true;
      }
      if (UnknownNBytes) {
        Event.fd_readwrite.nbytes = 1;
      } else {
        Event.fd_readwrite.nbytes = WriteBufSize - WriteBufUsed;
      }
      break;
    }
    }
  }

  for (uint64_t I = 0; I < NextTimerId; ++I) {
    struct kevent KEvent;
    EV_SET(&KEvent, I, EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
    ::kevent(Fd, &KEvent, 1, nullptr, 0, nullptr);
  }

  std::swap(FdDatas, OldFdDatas);
  FdDatas.clear();
  KEvents.clear();
  NextTimerId = 0;
  return;
}

void Poller::reset() noexcept {
  WasiEvents = {};
  Events.clear();
}

bool Poller::ok() noexcept { return FdHolder::ok(); }

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
