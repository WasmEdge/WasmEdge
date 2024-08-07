// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "executor/executor.h"
#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include "runtime/instance/module.h"
#include "system/winapi.h"
#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

#if !WASMEDGE_OS_WINDOWS
#include <netinet/in.h>
#else
using namespace WasmEdge::winapi;
#endif
using namespace std::literals;

namespace {

// XXX: Setup a socket with address ::1 to test if IPv6 is available.
//      It prevent system call like sysctl net.ipv6.conf.all.disable_ipv6
//      However The port used in TEST can not be same as TrySetUpIPV6Socket
//      Because It do not set up SO_REUSEADDR=1 may cause test into fail.
bool TrySetUpIPV6Socket() {
  bool State = false;

#if WASMEDGE_OS_WINDOWS
  WSADATA_ WSAData;
  WSAStartup(0x0202, &WSAData);
  const SOCKET_ ErrFd = INVALID_SOCKET_;
  SOCKET_ Fd;
#else
  const int ErrFd = -1;
  int Fd;
#endif

  do {
    Fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if (Fd == ErrFd)
      break;

    struct sockaddr_in6 Sock6;
    std::memset(&Sock6, 0, sizeof(Sock6));
    Sock6.sin6_family = AF_INET6;
    Sock6.sin6_port = htons(10000);
    Sock6.sin6_addr = in6addr_loopback;

    if (bind(Fd, reinterpret_cast<sockaddr *>(&Sock6), sizeof(Sock6)) < 0)
      break;

    State = true;
  } while (false);

#if WASMEDGE_OS_WINDOWS
  closesocket(Fd);
  WSACleanup();
#else
  close(Fd);
#endif

  return State;
}

bool TestIPv6Enabled() {
  static bool Resolved = false;
  static bool IPv6Enabled;

  if (!Resolved) {
    IPv6Enabled = TrySetUpIPV6Socket();
    Resolved = true;
  }

  return IPv6Enabled;
}

void writeDummyMemoryContent(
    WasmEdge::Runtime::Instance::MemoryInstance &MemInst) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}

void writeString(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 std::string_view String, uint32_t Ptr) noexcept {
  std::copy(String.begin(), String.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

void writeAddrinfo(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   __wasi_addrinfo_t *WasiAddrinfo, uint32_t Ptr) {
  std::memcpy(MemInst.getPointer<__wasi_addrinfo_t *>(Ptr), WasiAddrinfo,
              sizeof(__wasi_addrinfo_t));
}

void allocateAddrinfoArray(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t Base, uint32_t Length,
                           uint32_t CanonnameMaxSize) {
  for (uint32_t Item = 0; Item < Length; Item++) {
    // allocate addrinfo struct
    auto *ResItemPtr = MemInst.getPointer<__wasi_addrinfo_t *>(Base);
    Base += sizeof(__wasi_addrinfo_t);

    // allocate sockaddr struct
    ResItemPtr->ai_addr = Base;
    ResItemPtr->ai_addrlen = sizeof(__wasi_sockaddr_t);
    auto *Sockaddr =
        MemInst.getPointer<__wasi_sockaddr_t *>(ResItemPtr->ai_addr);
    Base += ResItemPtr->ai_addrlen;
    // allocate sockaddr sa_data.
    Sockaddr->sa_data = Base;
    Sockaddr->sa_data_len = WasmEdge::Host::WASI::kMaxSaDataLen;
    Base += Sockaddr->sa_data_len;
    // allocate ai_canonname
    ResItemPtr->ai_canonname = Base;
    ResItemPtr->ai_canonname_len = CanonnameMaxSize;
    Base += ResItemPtr->ai_canonname_len;
    if (Item != (Length - 1)) {
      ResItemPtr->ai_next = Base;
    }
  }
}
} // namespace

TEST(WasiSockTest, SocketUDP_4V1) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV1 WasiSockOpen(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockBindV1 WasiSockBind(Env);
  WasmEdge::Host::WasiSockSendToV1 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV1 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t AddrBufPtr = 100;
    uint32_t AddrBuflen = 4;
    uint32_t AddrPtr = 200;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_MAX);

    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr);
    *AddrBufSend = htonl(INADDR_LOOPBACK);
    Addr->buf_len = sizeof(uint32_t);

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    Addr->buf_len = 4;

    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 7>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
  // False SockType
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = 3;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, UINT32_C(0)},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
    Env.fini();
  }
  // False AddressFamily
  {
    uint32_t AddressFamily = UINT32_MAX;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, UINT32_C(0)},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
    Env.fini();
  }
  // Invalid Address Length for Bind
  {
    uint32_t Fd = 0;
    uint32_t Port = 12345;
    uint8_t_ptr AddrBufPtr = 100;
    uint32_t AddrBuflen = 7;
    uint32_t AddrPtr = 200;
    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }
  // Invalid Fd for Bind
  {
    uint32_t Fd = 0;
    uint32_t Port = 12345;
    uint8_t_ptr AddrBufPtr = 100;
    uint32_t AddrBuflen = 16;
    uint32_t AddrPtr = 200;
    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_BADF);
  }
}

TEST(WasiSockTest, SocketUDP_4V2) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockSendToV2 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV2 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t PortPtr = 20;
    uint32_t AddrPtr = 100;
    uint32_t AddrBufPtr = 200;
    uint32_t AddrBuflen = 128;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_MAX);

    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET4;
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET4;
    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr + 2);
    *AddrBufSend = htonl(INADDR_LOOPBACK);
    Addr->buf_len = 128; // sizeof(uint32_t);

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS)
      GTEST_SKIP();
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    Addr->buf_len = 128;

    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 8>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), PortPtr, RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
  // False SockType
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = 3;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, UINT32_C(0)},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
    Env.fini();
  }
  // False AddressFamily
  {
    uint32_t AddressFamily = UINT32_MAX;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, UINT32_C(0)},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
    Env.fini();
  }
  // Invalid Address Length for Bind
  {
    uint32_t Fd = 0;
    uint32_t Port = 12345;
    uint8_t_ptr AddrBufPtr = 100;
    uint32_t AddrBuflen = 7;
    uint32_t AddrPtr = 200;
    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }
  // Invalid Fd for Bind
  {
    uint32_t Fd = 0;
    uint32_t Port = 12345;
    uint8_t_ptr AddrPtr = 100;
    uint32_t AddrBuflen = 128;
    uint32_t AddrBufPtr = 200;
    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_BADF);
  }
}

TEST(WasiSockTest, SocketUDP_6) {
  if (!TestIPv6Enabled()) {
    GTEST_SKIP();
  }

  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockSendToV2 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV2 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t PortPtr = 20;
    uint32_t AddrPtr = 100;
    uint32_t AddrBuflen = 128;
    uint32_t AddrBufPtr = 200;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_MAX);

    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET6;
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge in ipv6."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET6;
    auto *AddrBufSend = MemInst.getPointer<in6_addr *>(AddrBufPtr + 2);
    *AddrBufSend = in6addr_loopback;
    Addr->buf_len = 128;

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS)
      GTEST_SKIP();

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    Addr->buf_len = 128;
    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 8>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), PortPtr, RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, SocketUDP_4_Fallback) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;

  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockSendToV2 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV2 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t PortPtr = 20;
    uint32_t AddrBufPtr = 100;
    uint32_t AddrBuflen = 4;
    uint32_t AddrPtr = 200;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_MAX);

    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr);
    *AddrBufSend = htonl(INADDR_LOOPBACK);
    Addr->buf_len = sizeof(uint32_t);

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    Addr->buf_len = 4;

    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 8>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), PortPtr, RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, SocketUDP_6_Fallback) {
  if (!TestIPv6Enabled()) {
    GTEST_SKIP();
  }

  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;

  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockSendToV2 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV2 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t PortPtr = 20;
    uint32_t AddrBufPtr = 100;
    uint32_t AddrBuflen = 16;
    uint32_t AddrPtr = 200;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_MAX);

    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge in ipv6."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    auto *AddrBufSend = MemInst.getPointer<in6_addr *>(AddrBufPtr);
    *AddrBufSend = in6addr_loopback;
    Addr->buf_len = sizeof(*AddrBufSend);

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 8>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), PortPtr, RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, SockOpt) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockGetOpt WasiSockGetOpt(Env);
  WasmEdge::Host::WasiSockSetOpt WasiSockSetOpt(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t FdPtr = 0;
    uint32_t ResBufSzPtr = 12;
    uint32_t ResBufPtr = 24;
    uint32_t ResMaxLen = 16;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{AddressFamily, SockType, FdPtr},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdPtr), UINT32_MAX);

    int32_t Fd = *MemInst.getPointer<const int32_t *>(FdPtr);

    uint32_t OptLevel =
        __wasi_sock_opt_level_t::__WASI_SOCK_OPT_LEVEL_SOL_SOCKET;
    uint32_t OptName = __wasi_sock_opt_so_t::__WASI_SOCK_OPT_SO_TYPE;

    auto ResBuf = MemInst.getSpan<uint8_t>(ResBufPtr, ResMaxLen);
    auto ResBufSz = MemInst.getPointer<uint32_t *>(ResBufSzPtr);
    *ResBufSz = ResMaxLen;
    std::fill_n(ResBuf.data(), ResBuf.size(), 0x00);

    WasiSockGetOpt.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 5>{
                           Fd, OptLevel, OptName, ResBufPtr, ResBufSzPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(ResBufPtr),
              __WASI_SOCK_TYPE_SOCK_DGRAM);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t FdPtr = 0;
    uint32_t ResBufSzPtr = 12;
    uint32_t ResBufPtr = 24;
    int32_t Opt = 1;
    uint32_t ResMaxLen = sizeof(Opt);

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{AddressFamily, SockType, FdPtr},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdPtr), UINT32_MAX);

    int32_t Fd = *MemInst.getPointer<const int32_t *>(FdPtr);

    const uint32_t OptLevel = __WASI_SOCK_OPT_LEVEL_SOL_SOCKET;
    const uint32_t OptName = __WASI_SOCK_OPT_SO_BROADCAST;

    auto ResBuf = MemInst.getPointer<decltype(&Opt)>(ResBufPtr);
    auto ResBufSz = MemInst.getPointer<uint32_t *>(ResBufSzPtr);
    *ResBufSz = ResMaxLen;
    ::memset(ResBuf, 0x00, ResMaxLen);

    WasiSockGetOpt.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 5>{
                           Fd, OptLevel, OptName, ResBufPtr, ResBufSzPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<decltype(&Opt)>(ResBufPtr), false);

    ResBuf[0] = true;
    WasiSockSetOpt.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 5>{
                           Fd, OptLevel, OptName, ResBufPtr, ResMaxLen},
                       Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    ::memset(ResBuf, 0x00, ResMaxLen);
    WasiSockGetOpt.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 5>{
                           Fd, OptLevel, OptName, ResBufPtr, ResBufSzPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_TRUE(
        static_cast<bool>(*MemInst.getPointer<decltype(&Opt)>(ResBufPtr)));

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, SockGetLocalAddr_4) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockGetLocalAddrV2 WasiSockGetLocalAddr(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t BindAddress = htonl(INADDR_LOOPBACK);
    uint32_t Port = 12345;
    uint32_t FdPtr = 0;
    uint32_t AddrPtr = 100;
    uint32_t AddrBuflen = 128;
    uint32_t AddrBufPtr = 200;
    uint32_t ResPortPtr = 1000;
    uint32_t ResAddrPtr = 1200;
    uint32_t ResAddrBufPtr = 1232;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{AddressFamily, SockType, FdPtr},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdPtr), UINT32_MAX);

    int32_t Fd = *MemInst.getPointer<const int32_t *>(FdPtr);

    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET4;
    *MemInst.getPointer<uint32_t *>(AddrBufPtr + 2) = BindAddress;

    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    *MemInst.getPointer<uint32_t *>(ResPortPtr) = UINT32_C(0);

    auto *ResAddr = MemInst.getPointer<__wasi_address_t *>(ResAddrPtr);

    auto ResAddrBuf = MemInst.getSpan<uint8_t>(ResAddrBufPtr, AddrBuflen);
    std::fill_n(ResAddrBuf.data(), ResAddrBuf.size(), 0x00);

    ResAddr->buf = ResAddrBufPtr;
    ResAddr->buf_len = 128;

    WasiSockGetLocalAddr.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{Fd, ResAddrPtr, ResPortPtr}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint16_t *>(ResAddrBufPtr),
              __WASI_ADDRESS_FAMILY_INET4);
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(ResAddrBufPtr + 2), BindAddress);
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(ResPortPtr), Port);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, SockGetLocalAddr_6) {
  if (!TestIPv6Enabled()) {
    GTEST_SKIP();
  }

  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiSockGetLocalAddrV2 WasiSockGetLocalAddr(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 12345;
    uint32_t FdPtr = 0;
    uint32_t AddrPtr = 100;
    uint32_t AddrBuflen = 128;
    uint32_t AddrBufPtr = 200;
    uint32_t ResPortPtr = 1000;
    uint32_t ResAddrPtr = 1200;
    uint32_t ResAddrBufPtr = 1232;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{AddressFamily, SockType, FdPtr},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdPtr), UINT32_MAX);

    int32_t Fd = *MemInst.getPointer<const int32_t *>(FdPtr);

    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_INET6;
    auto *AddrBuf = MemInst.getPointer<in6_addr *>(AddrBufPtr + 2);
    *AddrBuf = in6addr_loopback;

    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    *MemInst.getPointer<uint32_t *>(ResPortPtr) = UINT32_C(0);

    auto *ResAddr = MemInst.getPointer<__wasi_address_t *>(ResAddrPtr);

    auto ResAddrBuf = MemInst.getSpan<uint8_t>(ResAddrBufPtr, AddrBuflen);
    std::fill_n(ResAddrBuf.data(), ResAddrBuf.size(), 0x00);

    ResAddr->buf = ResAddrBufPtr;
    ResAddr->buf_len = 128;

    WasiSockGetLocalAddr.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{Fd, ResAddrPtr, ResPortPtr}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    EXPECT_EQ(*MemInst.getPointer<uint16_t *>(ResAddrBufPtr),
              __WASI_ADDRESS_FAMILY_INET6);
    const auto LHS =
        MemInst.getSpan<const char>(ResAddrBufPtr + 2, sizeof(in6_addr));
    const auto RHS = WasmEdge::Span<const char>(
        reinterpret_cast<const char *>(&in6addr_loopback), sizeof(in6_addr));
    for (uint32_t I = 0; I < sizeof(in6_addr); ++I) {
      EXPECT_EQ(LHS[I], RHS[I]);
    }
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(ResPortPtr), Port);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiSockTest, GetAddrinfo) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiSockGetAddrinfo WasiGetAddrinfo(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  uint32_t NodePtr = 0;
  uint32_t ServicePtr = 32;
  uint32_t HintsPtr = 48;
  uint32_t ResLengthPtr = 100;
  uint32_t ResultPtr = 104;
  std::string Node = "";
  std::string Service = "27015";
  uint32_t MaxLength = 10;
  uint32_t CanonnameMaxSize = 50;

  const uint32_t NodeLen = static_cast<uint32_t>(Node.size());
  const uint32_t ServiceLen = static_cast<uint32_t>(Service.size());

  __wasi_addrinfo_t Hints;
  std::memset(&Hints, 0, sizeof(Hints));
  Hints.ai_family = __WASI_ADDRESS_FAMILY_INET4;   // Allow IPv4
  Hints.ai_socktype = __WASI_SOCK_TYPE_SOCK_DGRAM; // Datagram socket
  Hints.ai_flags = __WASI_AIFLAGS_AI_PASSIVE;      // For wildcard IP address
  Hints.ai_protocol = __WASI_PROTOCOL_IPPROTO_UDP; // UDP protocol
  writeString(MemInst, Node, NodePtr);
  writeString(MemInst, Service, ServicePtr);
  writeAddrinfo(MemInst, &Hints, HintsPtr);
  auto *ResLength = MemInst.getPointer<uint32_t *>(ResLengthPtr);
  *ResLength = 0;
  auto *Result = MemInst.getPointer<uint8_t_ptr *>(ResultPtr);
  *Result = 108;
  // allocate Res Item;
  allocateAddrinfoArray(MemInst, *Result, MaxLength, CanonnameMaxSize);

  Env.init({}, "test"s, {}, {});
  // MaxLength == 0;
  {
    uint32_t TmpResMaxLength = 0;
    EXPECT_TRUE(WasiGetAddrinfo.run(CallFrame,
                                    std::initializer_list<WasmEdge::ValVariant>{
                                        NodePtr, NodeLen, ServicePtr,
                                        ServiceLen, HintsPtr, ResultPtr,
                                        TmpResMaxLength, ResLengthPtr},
                                    Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AIMEMORY);
  }
  // MemInst is nullptr
  {
    EXPECT_TRUE(
        WasiGetAddrinfo.run(WasmEdge::Runtime::CallingFrame(nullptr, nullptr),
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, NodeLen, ServicePtr, ServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }
  // Node and Service are all nullptr
  {
    uint32_t TmpNodeLen = 0;
    uint32_t TmpServiceLen = 0;
    EXPECT_TRUE(
        WasiGetAddrinfo.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, TmpNodeLen, ServicePtr, TmpServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AINONAME);
  }
  // node is nullptr, service is not nullptr
  {
    uint32_t TmpNodeLen = 0;
    EXPECT_TRUE(
        WasiGetAddrinfo.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, TmpNodeLen, ServicePtr, ServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    auto *Res = MemInst.getPointer<uint8_t_ptr *>(ResultPtr);

    auto *ResHead = MemInst.getPointer<__wasi_addrinfo_t *>(*Res);
    auto *ResItem = ResHead;
    EXPECT_NE(*ResLength, 0);
    for (uint32_t Idx = 0; Idx < *ResLength; Idx++) {
      EXPECT_NE(ResItem->ai_addrlen, 0);
      auto *TmpSockAddr =
          MemInst.getPointer<__wasi_sockaddr_t *>(ResItem->ai_addr);
      EXPECT_EQ(TmpSockAddr->sa_data_len, 14);
      EXPECT_EQ(MemInst.getSpan<char>(TmpSockAddr->sa_data,
                                      TmpSockAddr->sa_data_len)[0],
                'i');
      if (Idx != (*ResLength) - 1) {
        ResItem = MemInst.getPointer<__wasi_addrinfo_t *>(ResItem->ai_next);
      }
    }
  }
  allocateAddrinfoArray(MemInst, *Result, MaxLength, CanonnameMaxSize);
  // hints.ai_flag is ai_canonname but has an error
  {
    Hints.ai_flags = __WASI_AIFLAGS_AI_CANONNAME;
    writeAddrinfo(MemInst, &Hints, HintsPtr);
    EXPECT_TRUE(
        WasiGetAddrinfo.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, NodeLen, ServicePtr, ServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AIBADFLAG);
  }

  // node is nullptr, service is not nullptr
  {
    std::string TmpNode = "google.com";
    writeString(MemInst, TmpNode, NodePtr);
    uint32_t TmpNodeLen = static_cast<uint32_t>(TmpNode.size());
    EXPECT_TRUE(
        WasiGetAddrinfo.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, TmpNodeLen, ServicePtr, ServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*ResLength, 0);
    auto *Res = MemInst.getPointer<uint8_t_ptr *>(ResultPtr);

    auto *ResHead = MemInst.getPointer<__wasi_addrinfo_t *>(*Res);
    EXPECT_NE(ResHead->ai_canonname_len, 0);
    EXPECT_STREQ(MemInst
                     .getSpan<const char>(ResHead->ai_canonname,
                                          ResHead->ai_canonname_len)
                     .data(),
                 "google.com");
    auto *WasiSockAddr =
        MemInst.getPointer<__wasi_sockaddr_t *>(ResHead->ai_addr);
    EXPECT_EQ(WasiSockAddr->sa_data_len, 14);
  }
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// TODO: add af_unix for windows
#if WASMEDGE_OS_MACOS || WASMEDGE_OS_LINUX
TEST(WasiTest, UNIX_Socket) {
  WasmEdge::Configure Configure;
  Configure.getRuntimeConfigure().setAllowAFUNIX(true);
  WasmEdge::Executor::Executor Executor(Configure);
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(&Executor, &Mod);

  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiSockSendToV2 WasiSockSendTo(Env);
  WasmEdge::Host::WasiSockRecvFromV2 WasiSockRecvFrom(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  // Open and Close udp unix socket
  {
    uint32_t AddressFamily = __WASI_ADDRESS_FAMILY_AF_UNIX;
    uint32_t SockType = __WASI_SOCK_TYPE_SOCK_DGRAM;
    uint32_t Port = 0;
    uint32_t FdServerPtr = 0;
    uint32_t FdClientPtr = 4;
    uint32_t SendtoRetPtr = 8;
    uint32_t RecvfromRetPtr = 12;
    uint32_t FlagPtr = 16;
    uint32_t PortPtr = 20;
    uint32_t AddrPtr = 100;
    uint32_t AddrBufPtr = 200;
    uint32_t AddrBuflen = 128;
    uint32_t MsgInPackPtr = 900;
    uint32_t MsgInPtr = 1000;
    uint32_t MsgOutPackPtr = 1900;
    uint32_t MsgOutPtr = 2000;

    writeDummyMemoryContent(MemInst);
    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdServerPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdServerPtr), UINT32_C(-1));
    int32_t FdServer = *MemInst.getPointer<const int32_t *>(FdServerPtr);

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint32_t *>(FdClientPtr), UINT32_C(-1));

    int32_t FdClient = *MemInst.getPointer<const int32_t *>(FdClientPtr);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) = __WASI_ADDRESS_FAMILY_AF_UNIX;

    const std::string Path = "wasmedge_unix_socket_test.sock";
    writeString(MemInst, Path, AddrBufPtr + 2);
    Addr->buf = AddrBufPtr;
    Addr->buf_len = AddrBuflen;

#if WASMEDGE_OS_WINDOWS
    _unlink(Path.c_str());
#else
    unlink(Path.c_str());
#endif

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge in unix domain socket."sv;
    uint32_t Msg1Len = Msg1.size();
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = MsgInPtr;
    MsgInPack->buf_len = Msg1Len;

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr,
                           INT32_C(Port), UINT32_C(0), SendtoRetPtr},
                       Errno);

    if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS)
      GTEST_SKIP();
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), AddrBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = MsgOutPtr;
    MsgOutPack->buf_len = MaxMsgBufLen;

    Addr->buf_len = 128;

    WasiSockRecvFrom.run(CallFrame,
                         std::array<WasmEdge::ValVariant, 8>{
                             FdServer, MsgOutPackPtr, UINT32_C(1), AddrPtr,
                             UINT32_C(0), PortPtr, RecvfromRetPtr, FlagPtr},
                         Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    std::string_view MsgRecv{MsgBuf.data(), Msg1.size()};
    EXPECT_EQ(MsgRecv, Msg1);

    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdServer},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{FdClient},
                    Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    Env.fini();
  }
}
#endif
