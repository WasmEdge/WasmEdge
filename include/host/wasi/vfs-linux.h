// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/defines.h"

#if WASMEDGE_OS_LINUX
#include "common/span.h"
#include "host/wasi/vfs.h"
#include <climits>
#include <dirent.h>

namespace WasmEdge {
namespace Host {

class WasiFile : public WasiFileBase<WasiFile> {
public:
  friend class WasiFileBase<WasiFile>;
  using BaseT = WasiFileBase<WasiFile>;

  static WasiFile stdinFile() noexcept;

  static WasiFile stdoutFile() noexcept;

  static WasiFile stderrFile() noexcept;

  static WasiFile preopened(std::string Name, std::string RealPath) noexcept;

  friend void swap(WasiFile &LHS, WasiFile &RHS) noexcept;

  WasiFile(__wasi_rights_t R, __wasi_rights_t IR, int Fd,
           std::string PN) noexcept;

  WasiFile(WasiFile &&RHS) noexcept;

  WasiFile &operator=(WasiFile &&RHS) noexcept;

  ~WasiFile() noexcept;

private:
  int Fd = -1;
  DIR *Dir = nullptr;
  __wasi_dircookie_t DirCookie = 0;
  alignas(__wasi_dirent_t)
      std::array<uint8_t, sizeof(__wasi_dirent_t) + NAME_MAX + 1> DirentBuffer;
  Span<const uint8_t> DirentData;

  struct Poller;

  bool doOk() const noexcept;

  __wasi_errno_t doFdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t Len,
                            __wasi_advice_t Advice) noexcept;

  __wasi_errno_t doFdAllocate(__wasi_filesize_t Offset,
                              __wasi_filesize_t Len) noexcept;

  __wasi_errno_t doFdClose() noexcept;

  __wasi_errno_t doFdDatasync() noexcept;

  __wasi_errno_t doFdFdstatGet(__wasi_fdstat_t &FdStat) noexcept;

  __wasi_errno_t doFdFdstatSetFlags(__wasi_fdflags_t Flags) noexcept;

  __wasi_errno_t doFdFilestatGet(__wasi_filestat_t &Filestat) noexcept;

  __wasi_errno_t doFdFilestatSetSize(__wasi_filesize_t Size) noexcept;

  __wasi_errno_t doFdFilestatSetTimes(__wasi_timestamp_t ATim,
                                      __wasi_timestamp_t MTim,
                                      __wasi_fstflags_t FstFlags) noexcept;

  __wasi_errno_t doFdPread(Span<const Span<uint8_t>> IOVS,
                           __wasi_filesize_t Offset,
                           __wasi_size_t &NRead) noexcept;

  __wasi_errno_t doFdPwrite(Span<const Span<const uint8_t>> IOVS,
                            __wasi_filesize_t Offset,
                            __wasi_size_t &NWritten) noexcept;

  __wasi_errno_t doFdRead(Span<const Span<uint8_t>> IOVS,
                          __wasi_size_t &NRead) noexcept;

  __wasi_errno_t doFdReaddir(Span<uint8_t> Buf, __wasi_dircookie_t Cookie,
                             __wasi_size_t &Bufused) noexcept;

  __wasi_errno_t doFdSeek(__wasi_filedelta_t Offset, __wasi_whence_t Whence,
                          __wasi_filedelta_t &NewOffset) noexcept;

  __wasi_errno_t doFdSync() noexcept;

  __wasi_errno_t doFdTell(__wasi_filesize_t &Offset) noexcept;

  __wasi_errno_t doFdWrite(Span<const Span<const uint8_t>> IOVS,
                           __wasi_size_t &NWritten) noexcept;

  __wasi_errno_t doPathCreateDirectory(Span<const uint8_t> Path) noexcept;

  __wasi_errno_t doPathFilestatGet(__wasi_lookupflags_t Flags,
                                   Span<const uint8_t> Path,
                                   __wasi_filestat_t &Buf) noexcept;

  __wasi_errno_t doPathFilestatSetTimes(__wasi_lookupflags_t Flags,
                                        Span<const uint8_t> Path,
                                        __wasi_timestamp_t ATim,
                                        __wasi_timestamp_t MTim,
                                        __wasi_fstflags_t FstFlags) noexcept;

  static __wasi_errno_t doPathLink(WasiFile &Old, __wasi_lookupflags_t OldFlags,
                                   Span<const uint8_t> OldPath, WasiFile &New,
                                   Span<const uint8_t> NewPath) noexcept;

  WasiExpect<WasiFile> doPathOpen(Span<const uint8_t> Path,
                                  __wasi_lookupflags_t DirFlags,
                                  __wasi_oflags_t OFlags,
                                  __wasi_rights_t FsRightsBase,
                                  __wasi_rights_t FsRightsInheriting,
                                  __wasi_fdflags_t FdFlags) noexcept;

  __wasi_errno_t doPathReadlink(Span<const uint8_t> Path, Span<uint8_t> Buf,
                                __wasi_size_t &BufUsed) noexcept;

  __wasi_errno_t doPathRemoveDirectory(Span<const uint8_t> Path) noexcept;

  static __wasi_errno_t doPathRename(WasiFile &Old, Span<const uint8_t> OldPath,
                                     WasiFile &New,
                                     Span<const uint8_t> NewPath) noexcept;

  __wasi_errno_t doPathSymlink(Span<const uint8_t> OldPath,
                               Span<const uint8_t> NewPath) noexcept;

  __wasi_errno_t doPathUnlinkFile(Span<const uint8_t> Path) noexcept;

  __wasi_errno_t doSockRecv(Span<const Span<uint8_t>> RiData,
                            __wasi_riflags_t RiFlags, __wasi_size_t &RoDatalen,
                            __wasi_roflags_t &RoFlags) noexcept;

  __wasi_errno_t doSockSend(Span<const Span<const uint8_t>> SiData,
                            __wasi_siflags_t SiFlags,
                            __wasi_size_t &SoDatalen) noexcept;

  __wasi_errno_t doSockShutdown(__wasi_sdflags_t How) noexcept;
};

} // namespace Host
} // namespace WasmEdge

#endif
