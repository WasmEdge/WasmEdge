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

WasiExpect<INode> INode::open(std::string, __wasi_oflags_t, __wasi_fdflags_t,
                              uint8_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t, __wasi_filesize_t,
                                 __wasi_advice_t) const noexcept {
  return {};
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t,
                                   __wasi_filesize_t) const noexcept {
  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept { return {}; }

WasiExpect<void> INode::fdFdstatGet(__wasi_fdstat_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdFdstatSetFlags(__wasi_fdflags_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdFilestatGet(__wasi_filestat_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdFilestatSetSize(__wasi_filesize_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdFilestatSetTimes(__wasi_timestamp_t,
                                           __wasi_timestamp_t,
                                           __wasi_fstflags_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdPread(Span<Span<uint8_t>>, __wasi_filesize_t,
                                __wasi_size_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdPwrite(Span<Span<const uint8_t>>, __wasi_filesize_t,
                                 __wasi_size_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdRead(Span<Span<uint8_t>> IOVs,
                               __wasi_size_t &NRead) const noexcept {
  NRead = 0;
  for (auto IOV : IOVs) {
    winapi::DWORD_ NumberOfBytesRead = 0;
    if (!winapi::ReadFile(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                          &NumberOfBytesRead, nullptr)) {
      return WasiUnexpect(fromLastError(winapi::GetLastError()));
    }
    NRead += NumberOfBytesRead;
  }
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdReaddir(Span<uint8_t>, __wasi_dircookie_t,
                                  __wasi_size_t &) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdSeek(__wasi_filedelta_t, __wasi_whence_t,
                               __wasi_filesize_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdSync() const noexcept { return {}; }

WasiExpect<void> INode::fdTell(__wasi_filesize_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdWrite(Span<Span<const uint8_t>> IOVs,
                                __wasi_size_t &NWritten) const noexcept {
  NWritten = 0;
  for (auto IOV : IOVs) {
    winapi::DWORD_ NumberOfBytesWritten = 0;
    if (!winapi::WriteFile(Handle, IOV.data(),
                           static_cast<uint32_t>(IOV.size()),
                           &NumberOfBytesWritten, nullptr)) {
      return WasiUnexpect(fromLastError(winapi::GetLastError()));
    }
    NWritten += NumberOfBytesWritten;
  }
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathCreateDirectory(std::string) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathFilestatGet(std::string,
                                        __wasi_filestat_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathFilestatSetTimes(std::string, __wasi_timestamp_t,
                                             __wasi_timestamp_t,
                                             __wasi_fstflags_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathLink(const INode &, std::string, const INode &,
                                 std::string) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<INode> INode::pathOpen(std::string, __wasi_oflags_t,
                                  __wasi_fdflags_t, uint8_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathReadlink(std::string, Span<char>,
                                     __wasi_size_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathRemoveDirectory(std::string) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathRename(const INode &, std::string, const INode &,
                                   std::string) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathSymlink(std::string, std::string) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::pathUnlinkFile(std::string) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<Poller> INode::pollOneoff(__wasi_size_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>>, __wasi_riflags_t,
                                 __wasi_size_t &,
                                 __wasi_roflags_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>>, __wasi_siflags_t,
                                 __wasi_size_t &) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}
WasiExpect<void> INode::getAddrinfo(const char *, const char *,
                                    const __wasi_addrinfo_t &, uint32_t,
                                    std::vector<struct __wasi_addrinfo_t *> *,
                                    std::vector<struct __wasi_sockaddr_t *> *,
                                    std::vector<char *> *,
                                    std::vector<char *> *,
                                    /*Out*/ __wasi_size_t *) noexcept {
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

WasiExpect<void> Poller::clock(__wasi_clockid_t, __wasi_timestamp_t,
                               __wasi_timestamp_t, __wasi_subclockflags_t,
                               __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::read(const INode &, __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::write(const INode &, __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::wait(CallbackType) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
