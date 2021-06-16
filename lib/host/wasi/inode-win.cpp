// SPDX-License-Identifier: Apache-2.0
#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "win.h"

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {

namespace winapi = boost::winapi;

} // namespace

void HandleHolder::reset() noexcept {
  if (likely(ok())) {
    winapi::CloseHandle(Handle);
    Handle = nullptr;
  }
}

INode INode::stdIn() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_INPUT_HANDLE_));
}

INode INode::stdOut() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_OUTPUT_HANDLE_));
}

INode INode::stdErr() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_ERROR_HANDLE_));
}

WasiExpect<INode> INode::open(std::string Path, __wasi_oflags_t OpenFlags,
                              __wasi_fdflags_t FdFlags,
                              uint8_t VFSFlags) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t Offset,
                                 __wasi_filesize_t Len,
                                 __wasi_advice_t Advice) const noexcept {
  return {};
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t Offset,
                                   __wasi_filesize_t Len) const noexcept {
  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept { return {}; }

WasiExpect<void> INode::fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::fdFilestatSetSize(__wasi_filesize_t Size) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                          __wasi_fstflags_t FstFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdPread(Span<Span<uint8_t>> IOVs,
                                __wasi_filesize_t Offset,
                                __wasi_size_t &NRead) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdPwrite(Span<Span<const uint8_t>> IOVs,
                                 __wasi_filesize_t Offset,
                                 __wasi_size_t &NWritten) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdRead(Span<Span<uint8_t>> IOVs,
                               __wasi_size_t &NRead) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdReaddir(Span<uint8_t> Buffer,
                                  __wasi_dircookie_t Cookie,
                                  __wasi_size_t &Size) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdSeek(__wasi_filedelta_t Offset,
                               __wasi_whence_t Whence,
                               __wasi_filesize_t &Size) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdSync() const noexcept { return {}; }

WasiExpect<void> INode::fdTell(__wasi_filesize_t &Size) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdWrite(Span<Span<const uint8_t>> IOVs,
                                __wasi_size_t &NWritten) const noexcept {
  for (auto IOV : IOVs) {
    winapi::DWORD_ NumberOfBytesWritten = 0;
    if (!winapi::WriteFile(Handle, IOV.data(), IOV.size(),
                           &NumberOfBytesWritten, nullptr)) {
      return WasiUnexpect(fromLastError(winapi::GetLastError()));
    }
    NWritten += NumberOfBytesWritten;
  }
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathCreateDirectory(std::string Path) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::pathFilestatGet(std::string Path,
                       __wasi_filestat_t &Filestat) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void>
INode::pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                            __wasi_timestamp_t MTim,
                            __wasi_fstflags_t FstFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathLink(const INode &Old, std::string OldPath,
                                 const INode &New,
                                 std::string NewPath) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<INode> INode::pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                                  __wasi_fdflags_t FdFlags,
                                  uint8_t VFSFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathReadlink(std::string Path,
                                     Span<char> Buffer) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathRemoveDirectory(std::string Path) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathRename(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathSymlink(std::string OldPath,
                                    std::string NewPath) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathUnlinkFile(std::string Path) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<Poller> INode::pollOneoff(__wasi_size_t NSubscriptions) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t SiFlags,
                                 __wasi_size_t &NWritten) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

__wasi_filetype_t INode::unsafeFiletype() const noexcept {
  return __WASI_FILETYPE_UNKNOWN;
}

WasiExpect<__wasi_filetype_t> INode::filetype() const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

bool INode::isDirectory() const noexcept { return false; }

bool INode::isSymlink() const noexcept { return false; }

WasiExpect<__wasi_filesize_t> INode::filesize() const noexcept { return false; }

bool INode::canBrowse() const noexcept { return false; }

Poller::Poller(__wasi_size_t Count) { Events.reserve(Count); }

WasiExpect<void> Poller::clock(__wasi_clockid_t Clock,
                               __wasi_timestamp_t Timeout,
                               __wasi_timestamp_t Precision,
                               __wasi_subclockflags_t Flags,
                               __wasi_userdata_t UserData) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::read(const INode &Fd,
                              __wasi_userdata_t UserData) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::write(const INode &Fd,
                               __wasi_userdata_t UserData) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::wait(CallbackType Callback) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
