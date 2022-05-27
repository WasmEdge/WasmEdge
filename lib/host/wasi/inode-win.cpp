// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "win.h"

#include <winsock2.h>
#include <ws2tcpip.h>

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {

namespace winapi = boost::winapi;

inline constexpr __wasi_size_t
calculateAddrinfoLinkedListSize(struct addrinfo *const Addrinfo) {
  __wasi_size_t Length = 0;
  for (struct addrinfo *TmpPointer = Addrinfo; TmpPointer != nullptr;
       TmpPointer = TmpPointer->ai_next) {
    Length++;
  }
  return Length;
};

static bool isSocket(LPVOID H) {
  if (likely(::GetFileType(H) != FILE_TYPE_PIPE)) {
    return false;
  }
  return !::GetNamedPipeInfo(H, nullptr, nullptr, nullptr, nullptr);
}

static SOCKET toSocket(boost::winapi::HANDLE_ H) {
  return reinterpret_cast<SOCKET>(H);
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

void HandleHolder::reset() noexcept {
  if (likely(ok())) {
    if (likely(!isSocket(&Handle))) {
      winapi::CloseHandle(Handle);
    } else {
      ::closesocket(reinterpret_cast<SOCKET>(Handle));
    }
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
  return {};
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
  return {};
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

static bool EnsureWSAStartup() {
  static bool WSALoad = false;
  static WSADATA WSAData;

  if (!WSALoad) {
    int Err = WSAStartup(MAKEWORD(2, 2), &WSAData);
    if (Err == 0) {
      WSALoad = true;
    }
  }

  return WSALoad;
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
  SysHint.ai_protocol = toProtocal(Hint.ai_protocol);
  SysHint.ai_addrlen = Hint.ai_addrlen;
  SysHint.ai_addr = nullptr;
  SysHint.ai_canonname = nullptr;
  SysHint.ai_next = nullptr;

  struct addrinfo *SysResPtr = nullptr;
  if (auto Res = ::getaddrinfo(NodeCStr, ServiceCStr, &SysHint, &SysResPtr);
      unlikely(Res != 0)) {
    // By MSDN, on failure, getaddrinfo returns a nonzero Windows Sockets error
    // code.
    return WasiUnexpect(fromWSAToEAIError(Res));
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
    CurAddrinfo->ai_protocol = fromProtocal(SysResItem->ai_protocol);
    CurAddrinfo->ai_family = fromAddressFamily(SysResItem->ai_family);
    CurAddrinfo->ai_addrlen = static_cast<uint32_t>(SysResItem->ai_addrlen);

    // process ai_canonname in addrinfo
    if (SysResItem->ai_canonname != nullptr) {
      CurAddrinfo->ai_canonname_len =
          static_cast<uint32_t>(std::strlen(SysResItem->ai_canonname));
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
      std::memcpy(AiAddrSaDataArray[Idx], SysResItem->ai_addr->sa_data,
                  WASI::kSaDataLen);
      CurSockaddr->sa_data_len = WASI::kSaDataLen;
    }
    // process ai_next in addrinfo
    SysResItem = SysResItem->ai_next;
  }
  ::freeaddrinfo(SysResPtr);

  return {};
}

WasiExpect<INode> INode::sockOpen(__wasi_address_family_t AddressFamily,
                                  __wasi_sock_type_t SockType) noexcept {
  EnsureWSAStartup();

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

  if (auto NewSock = ::socket(SysDomain, SysType, SysProtocol);
      unlikely(NewSock == INVALID_SOCKET)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    INode New(reinterpret_cast<boost::winapi::HANDLE_>(NewSock));
    return New;
  }
}

WasiExpect<void> INode::sockBind(uint8_t *Address, uint8_t AddressLength,
                                 uint16_t Port) noexcept {
  EnsureWSAStartup();

  if (AddressLength == 4) {
    struct sockaddr_in ServerAddr;
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    std::memcpy(&ServerAddr.sin_addr.s_addr, Address, AddressLength);

    if (auto Res = ::bind(toSocket(Handle),
                          reinterpret_cast<struct sockaddr *>(&ServerAddr),
                          sizeof(ServerAddr));
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  } else if (AddressLength == 16) {
    struct sockaddr_in6 ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));

    ServerAddr.sin6_family = AF_INET6;
    ServerAddr.sin6_port = htons(Port);
    std::memcpy(ServerAddr.sin6_addr.s6_addr, Address, AddressLength);
    if (auto Res = ::bind(toSocket(Handle),
                          reinterpret_cast<struct sockaddr *>(&ServerAddr),
                          sizeof(ServerAddr));
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  }
  return {};
}

WasiExpect<void> INode::sockListen(int32_t Backlog) noexcept {
  EnsureWSAStartup();
  if (auto Res = ::listen(toSocket(Handle), Backlog);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }
  return {};
}

WasiExpect<INode> INode::sockAccept() noexcept {
  EnsureWSAStartup();
  struct sockaddr_in ServerSocketAddr;
  ServerSocketAddr.sin_family = AF_INET;
  ServerSocketAddr.sin_addr.s_addr = INADDR_ANY;
  socklen_t AddressLen = sizeof(ServerSocketAddr);

  if (auto NewSock = ::accept(
          toSocket(Handle),
          reinterpret_cast<struct sockaddr *>(&ServerSocketAddr), &AddressLen);
      unlikely(NewSock == INVALID_SOCKET)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    INode New(reinterpret_cast<boost::winapi::HANDLE_>(NewSock));
    return New;
  }
}

WasiExpect<void> INode::sockConnect(uint8_t *Address, uint8_t AddressLength,
                                    uint16_t Port) noexcept {
  EnsureWSAStartup();
  if (AddressLength == 4) {
    struct sockaddr_in ClientSocketAddr;
    ClientSocketAddr.sin_family = AF_INET;
    ClientSocketAddr.sin_port = htons(Port);
    std::memcpy(&ClientSocketAddr.sin_addr.s_addr, Address, AddressLength);

    if (auto Res =
            ::connect(toSocket(Handle),
                      reinterpret_cast<struct sockaddr *>(&ClientSocketAddr),
                      sizeof(ClientSocketAddr));
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  } else if (AddressLength == 16) {
    struct sockaddr_in6 ClientSocketAddr;

    ClientSocketAddr.sin6_family = AF_INET6;
    ClientSocketAddr.sin6_port = htons(Port);
    std::memcpy(ClientSocketAddr.sin6_addr.s6_addr, Address, AddressLength);
    if (auto Res =
            ::bind(toSocket(Handle),
                   reinterpret_cast<struct sockaddr *>(&ClientSocketAddr),
                   sizeof(ClientSocketAddr));
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  }
  return {};
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  return sockRecvFrom(RiData, RiFlags, nullptr, 0, NRead, RoFlags);
}

WasiExpect<void> INode::sockRecvFrom(Span<Span<uint8_t>> RiData,
                                     __wasi_riflags_t RiFlags, uint8_t *Address,
                                     uint8_t AddressLength,
                                     __wasi_size_t &NRead,
                                     __wasi_roflags_t &RoFlags) const noexcept {
  EnsureWSAStartup();
  // recvmsg is not available on WINDOWS. fall back to call recvfrom
  int SysRiFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    SysRiFlags |= MSG_PEEK;
  }
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    SysRiFlags |= MSG_WAITALL;
  }

  std::size_t TmpBufSize = 0;
  for (auto &IOV : RiData) {
    TmpBufSize += IOV.size();
  }

  std::vector<uint8_t> TmpBuf(TmpBufSize, 0);
  int AddrLen = AddressLength;

  if (auto Res =
          ::recvfrom(toSocket(Handle), reinterpret_cast<char *>(TmpBuf.data()),
                     static_cast<int>(TmpBufSize), SysRiFlags,
                     reinterpret_cast<sockaddr *>(Address), &AddrLen);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    NRead = static_cast<__wasi_size_t>(Res);
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);
  // TODO : check MSG_TRUNC

  size_t BeginIdx = 0;
  for (auto &IOV : RiData) {
    std::copy(TmpBuf.data() + BeginIdx, TmpBuf.data() + BeginIdx + IOV.size(),
              IOV.begin());
    BeginIdx += IOV.size();
  }

  return {};
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t SiFlags,
                                 __wasi_size_t &NWritten) const noexcept {
  return sockSendTo(SiData, SiFlags, nullptr, 0, 0, NWritten);
}
WasiExpect<void> INode::sockSendTo(Span<Span<const uint8_t>> SiData,
                                   __wasi_siflags_t, uint8_t *Address,
                                   uint8_t AddressLength, int32_t Port,
                                   __wasi_size_t &NWritten) const noexcept {
  EnsureWSAStartup();
  // sendmsg is not available on WINDOWS. fall back to call sendto
  int SysSiFlags = 0;

  std::vector<uint8_t> TmpBuf;
  for (auto &IOV : SiData) {
    copy(IOV.begin(), IOV.end(), std::back_inserter(TmpBuf));
  }
  std::size_t TmpBufSize = TmpBuf.size();

  struct sockaddr_in ClientSocketAddr;
  struct sockaddr_in6 ClientSocketAddr6;
  void *Addr = nullptr;
  socklen_t AddrLen = 0;

  if (Address) {
    if (AddressLength == 4) {
      ClientSocketAddr.sin_family = AF_INET;
      ClientSocketAddr.sin_port = htons(static_cast<u_short>(Port));
      ::memcpy(&ClientSocketAddr.sin_addr.s_addr, Address, AddressLength);

      Addr = &ClientSocketAddr;
      AddrLen = sizeof(ClientSocketAddr);
    } else if (AddressLength == 16) {
      ClientSocketAddr6.sin6_family = AF_INET6;
      ClientSocketAddr6.sin6_port = htons(static_cast<u_short>(Port));
      ::memcpy(&ClientSocketAddr6.sin6_addr.s6_addr, Address, AddressLength);

      Addr = &ClientSocketAddr6;
      AddrLen = sizeof(ClientSocketAddr6);
    }
  }

  if (auto Res =
          ::sendto(toSocket(Handle), reinterpret_cast<char *>(TmpBuf.data()),
                   static_cast<int>(TmpBufSize), SysSiFlags,
                   reinterpret_cast<sockaddr *>(Addr), AddrLen);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    NWritten = static_cast<__wasi_size_t>(Res);
  }

  return {};
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
  EnsureWSAStartup();
  int SysFlags = 0;
  if (SdFlags == __WASI_SDFLAGS_RD) {
    SysFlags = SD_RECEIVE;
  } else if (SdFlags == __WASI_SDFLAGS_WR) {
    SysFlags = SD_SEND;
  } else if (SdFlags == (__WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR)) {
    SysFlags = SD_BOTH;
  }

  if (auto Res = ::shutdown(toSocket(Handle), SysFlags);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  return {};
}

WasiExpect<void> INode::sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   void *FlagPtr,
                                   uint32_t *FlagSizePtr) const noexcept {
  EnsureWSAStartup();
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  auto UnsafeFlagSizePtr = reinterpret_cast<int *>(FlagSizePtr);
  if (SockOptName == __WASI_SOCK_OPT_SO_ERROR) {
    char ErrorCode = 0;
    int *WasiErrorPtr = static_cast<int *>(FlagPtr);
    if (auto Res = ::getsockopt(toSocket(Handle), SysSockOptLevel,
                                SysSockOptName, &ErrorCode, UnsafeFlagSizePtr);
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
    *WasiErrorPtr = fromErrNo(ErrorCode);
  } else {
    char *CFlagPtr = static_cast<char *>(FlagPtr);
    if (auto Res = ::getsockopt(toSocket(Handle), SysSockOptLevel,
                                SysSockOptName, CFlagPtr, UnsafeFlagSizePtr);
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  }

  return {};
}

WasiExpect<void> INode::sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   void *FlagPtr,
                                   uint32_t FlagSize) const noexcept {
  EnsureWSAStartup();
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  char *CFlagPtr = static_cast<char *>(FlagPtr);
  auto UnsafeFlagSize = static_cast<int>(FlagSize);

  if (auto Res = ::setsockopt(toSocket(Handle), SysSockOptLevel, SysSockOptName,
                              CFlagPtr, UnsafeFlagSize);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  return {};
}

WasiExpect<void> INode::sockGetLoaclAddr(uint8_t *AddressPtr,
                                         uint32_t *AddrTypePtr,
                                         uint32_t *PortPtr) const noexcept {
  EnsureWSAStartup();
  struct sockaddr_storage SocketAddr;
  socklen_t Slen = sizeof(SocketAddr);
  std::memset(&SocketAddr, 0, sizeof(SocketAddr));

  if (auto Res = ::getsockname(
          toSocket(Handle), reinterpret_cast<sockaddr *>(&SocketAddr), &Slen);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  size_t AddrLen = 4;
  if (Slen != 16) {
    AddrLen = 16;
  }

  if (SocketAddr.ss_family == AF_INET) {
    *AddrTypePtr = 4;
    auto SocketAddrv4 = reinterpret_cast<struct sockaddr_in *>(&SocketAddr);
    *PortPtr = ntohs(SocketAddrv4->sin_port);
    std::memcpy(AddressPtr, &(SocketAddrv4->sin_addr.s_addr), AddrLen);
  } else if (SocketAddr.ss_family == AF_INET6) {
    *AddrTypePtr = 6;
    auto SocketAddrv6 = reinterpret_cast<struct sockaddr_in6 *>(&SocketAddr);
    *PortPtr = ntohs(SocketAddrv6->sin6_port);
    std::memcpy(AddressPtr, SocketAddrv6->sin6_addr.s6_addr, AddrLen);
  } else {
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }

  return {};
}

WasiExpect<void> INode::sockGetPeerAddr(uint8_t *, uint32_t *,
                                        uint32_t *) const noexcept {
  EnsureWSAStartup();
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
