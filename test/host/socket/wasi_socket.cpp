// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "common/types.h"
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

// XXX: Set up a socket with address ::1 to test if IPv6 is available.
//      It prevents system calls like sysctl net.ipv6.conf.all.disable_ipv6.
//      However, the port used in TEST cannot be the same as
//      TrySetUpIPV6Socket because it does not set up SO_REUSEADDR=1 and may
//      cause the test to fail.
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

template <typename T> void unalignedCopy(T *Dest, const T &Src) noexcept {
  auto DestSpan = as_writable_bytes(WasmEdge::Span<T, 1>(Dest, 1));
  auto SrcSpan = as_bytes(WasmEdge::Span<const T, 1>(&Src, 1));
  std::copy(SrcSpan.begin(), SrcSpan.end(), DestSpan.begin());
}

template <typename T> T unalignedRead(const T *Src) noexcept {
  T Dest;
  auto DestSpan = as_writable_bytes(WasmEdge::Span<T, 1>(&Dest, 1));
  auto SrcSpan = as_bytes(WasmEdge::Span<const T, 1>(Src, 1));
  std::copy(SrcSpan.begin(), SrcSpan.end(), DestSpan.begin());
  return Dest;
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

struct AddrinfoTestData {
  static constexpr uint32_t NodePtr = 0;
  static constexpr uint32_t ServicePtr = 32;
  static constexpr uint32_t HintsPtr = 64;
  static constexpr uint32_t ResultPtr = 96;
  static constexpr uint32_t ResultLengthPtr = 100;
  static constexpr uint32_t AddrinfoPtr = 128;
  static constexpr uint32_t SockaddrPtr = 192;
  static constexpr uint32_t SocketDataPtr = 224;
  static constexpr uint32_t CanonnamePtr = 256;
  static constexpr uint32_t InitialResultLength = 0xa5a5a5a5;

  __wasi_addrinfo_t Addrinfo;
  __wasi_sockaddr_t Sockaddr;
};

AddrinfoTestData prepareAddrinfoTest(
    WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
    uint32_t SocketDataCapacity, uint32_t CanonnameCapacity,
    __wasi_aiflags_t Flags, std::string_view Node = "127.0.0.1",
    __wasi_address_family_t Family = __WASI_ADDRESS_FAMILY_INET4) {
  const std::string_view Service = "80";
  writeString(MemInst, Node, AddrinfoTestData::NodePtr);
  writeString(MemInst, Service, AddrinfoTestData::ServicePtr);

  __wasi_addrinfo_t Hints{};
  Hints.ai_flags = Flags;
  Hints.ai_family = Family;
  Hints.ai_socktype = __WASI_SOCK_TYPE_SOCK_STREAM;
  Hints.ai_protocol = __WASI_PROTOCOL_IPPROTO_IP;
  writeAddrinfo(MemInst, &Hints, AddrinfoTestData::HintsPtr);

  AddrinfoTestData Data{};
  Data.Addrinfo.ai_flags = static_cast<__wasi_aiflags_t>(0x5a);
  Data.Addrinfo.ai_family = __WASI_ADDRESS_FAMILY_UNSPEC;
  Data.Addrinfo.ai_socktype = __WASI_SOCK_TYPE_SOCK_ANY;
  Data.Addrinfo.ai_protocol = __WASI_PROTOCOL_IPPROTO_IP;
  Data.Addrinfo.ai_addrlen = sizeof(__wasi_sockaddr_t);
  Data.Addrinfo.ai_addr = AddrinfoTestData::SockaddrPtr;
  Data.Addrinfo.ai_canonname = AddrinfoTestData::CanonnamePtr;
  Data.Addrinfo.ai_canonname_len = CanonnameCapacity;
  writeAddrinfo(MemInst, &Data.Addrinfo, AddrinfoTestData::AddrinfoPtr);

  Data.Sockaddr.sa_family = __WASI_ADDRESS_FAMILY_UNSPEC;
  Data.Sockaddr.sa_data = AddrinfoTestData::SocketDataPtr;
  Data.Sockaddr.sa_data_len = SocketDataCapacity;
  std::memcpy(
      MemInst.getPointer<__wasi_sockaddr_t *>(AddrinfoTestData::SockaddrPtr),
      &Data.Sockaddr, sizeof(Data.Sockaddr));

  *MemInst.getPointer<uint8_t_ptr *>(AddrinfoTestData::ResultPtr) =
      AddrinfoTestData::AddrinfoPtr;
  *MemInst.getPointer<uint32_t *>(AddrinfoTestData::ResultLengthPtr) =
      AddrinfoTestData::InitialResultLength;
  return Data;
}

void expectAddrinfoFieldsEqual(const __wasi_addrinfo_t &Actual,
                               const __wasi_addrinfo_t &Expected) {
  EXPECT_EQ(Actual.ai_flags, Expected.ai_flags);
  EXPECT_EQ(Actual.ai_family, Expected.ai_family);
  EXPECT_EQ(Actual.ai_socktype, Expected.ai_socktype);
  EXPECT_EQ(Actual.ai_protocol, Expected.ai_protocol);
  EXPECT_EQ(Actual.ai_addrlen, Expected.ai_addrlen);
  EXPECT_EQ(Actual.ai_addr, Expected.ai_addr);
  EXPECT_EQ(Actual.ai_canonname, Expected.ai_canonname);
  EXPECT_EQ(Actual.ai_canonname_len, Expected.ai_canonname_len);
  EXPECT_EQ(Actual.ai_next, Expected.ai_next);
}

void expectSockaddrFieldsEqual(const __wasi_sockaddr_t &Actual,
                               const __wasi_sockaddr_t &Expected) {
  EXPECT_EQ(Actual.sa_family, Expected.sa_family);
  EXPECT_EQ(Actual.sa_data_len, Expected.sa_data_len);
  EXPECT_EQ(Actual.sa_data, Expected.sa_data);
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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr);
    *AddrBufSend = htonl(INADDR_LOOPBACK);
    Addr->buf_len =
        WasmEdge::EndianValue(static_cast<__wasi_size_t>(sizeof(uint32_t)))
            .le();

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

    Addr->buf_len = WasmEdge::EndianValue(UINT32_C(4)).le();

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
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

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
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET4))
            .le();
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET4))
            .le();
    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr + 2);
    unalignedCopy(AddrBufSend, static_cast<uint32_t>(htonl(INADDR_LOOPBACK)));
    Addr->buf_len =
        WasmEdge::EndianValue(UINT32_C(128)).le(); // sizeof(uint32_t);

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    const auto SendErrno = Errno[0].get<int32_t>();
    if (SendErrno == __WASI_ERRNO_ACCES || SendErrno == __WASI_ERRNO_PERM)
      GTEST_SKIP() << "sock_send_to blocked with WASI errno=" << SendErrno;
    ASSERT_EQ(SendErrno, __WASI_ERRNO_SUCCESS);
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

    Addr->buf_len = WasmEdge::EndianValue(128).le();

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
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

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
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET6))
            .le();
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge in ipv6."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET6))
            .le();
    auto *AddrBufSend = MemInst.getPointer<in6_addr *>(AddrBufPtr + 2);
    *AddrBufSend = in6addr_loopback;
    Addr->buf_len = WasmEdge::EndianValue(128).le();

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    const auto SendErrno = Errno[0].get<int32_t>();
    if (SendErrno == __WASI_ERRNO_ACCES || SendErrno == __WASI_ERRNO_PERM)
      GTEST_SKIP() << "sock_send_to blocked with WASI errno=" << SendErrno;
    ASSERT_EQ(SendErrno, __WASI_ERRNO_SUCCESS);

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

    Addr->buf_len = WasmEdge::EndianValue(128).le();
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

TEST(WasiSockTest, SockConnect_6) {
  if (!TestIPv6Enabled()) {
    GTEST_SKIP();
  }

#if WASMEDGE_OS_WINDOWS
  WSADATA_ WSAData;
  WSAStartup(0x0202, &WSAData);
  const SOCKET_ ErrFd = INVALID_SOCKET_;
  SOCKET_ ServerFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#else
  const int ErrFd = -1;
  int ServerFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#endif

  ASSERT_NE(ServerFd, ErrFd);

  const int One = 1;
  ASSERT_EQ(setsockopt(ServerFd, SOL_SOCKET, SO_REUSEADDR,
                       reinterpret_cast<const char *>(&One),
                       static_cast<int>(sizeof(One))),
            0);

  sockaddr_in6 ServerAddr{};
  ServerAddr.sin6_family = AF_INET6;
  ServerAddr.sin6_port = htons(0);
  ServerAddr.sin6_addr = in6addr_loopback;

  ASSERT_EQ(bind(ServerFd, reinterpret_cast<sockaddr *>(&ServerAddr),
                 sizeof(ServerAddr)),
            0);
  ASSERT_EQ(listen(ServerFd, 1), 0);

  sockaddr_in6 BoundAddr{};
#if WASMEDGE_OS_WINDOWS
  int BoundLen = sizeof(BoundAddr);
#else
  socklen_t BoundLen = sizeof(BoundAddr);
#endif
  ASSERT_EQ(getsockname(ServerFd, reinterpret_cast<sockaddr *>(&BoundAddr),
                        &BoundLen),
            0);
  const uint16_t Port = ntohs(BoundAddr.sin6_port);

  std::promise<void> ServerReady;
  auto ReadyFuture = ServerReady.get_future();

  std::thread AcceptThread([&]() {
    // Signal main thread that server thread is scheduled and running
    ServerReady.set_value();
    auto ConnFd = accept(ServerFd, nullptr, nullptr);
    if (ConnFd != ErrFd) {
#if WASMEDGE_OS_WINDOWS
      closesocket(ConnFd);
#else
      close(ConnFd);
#endif
    }
  });

  ReadyFuture.wait();

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
  WasmEdge::Host::WasiSockConnectV1 WasiSockConnect(Env);
  WasmEdge::Host::WasiFdClose WasiFdClose(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  const uint32_t FdPtr = 0;
  const uint32_t AddrPtr = 16;
  const uint32_t AddrBufPtr = 64;
  const uint32_t AddrBufLen = 16;

  Env.init({}, "test"s, {}, {});

  EXPECT_TRUE(WasiSockOpen.run(
      CallFrame,
      std::array<WasmEdge::ValVariant, 3>{
          static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET6),
          static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

  int32_t Fd =
      WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdPtr)).le();

  auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);
  auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBufLen);
  std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
  AddrBuf[AddrBufLen - 1] = 1;

  Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
  Addr->buf_len = WasmEdge::EndianValue(AddrBufLen).le();

  EXPECT_TRUE(WasiSockConnect.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

  WasiFdClose.run(CallFrame, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno);
  Env.fini();

#if WASMEDGE_OS_WINDOWS
  closesocket(ServerFd);
  AcceptThread.join();
  WSACleanup();
#else
  close(ServerFd);
  AcceptThread.join();
#endif
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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    auto *AddrBufSend = MemInst.getPointer<uint32_t *>(AddrBufPtr);
    *AddrBufSend = htonl(INADDR_LOOPBACK);
    Addr->buf_len =
        WasmEdge::EndianValue(static_cast<uint32_t>(sizeof(uint32_t))).le();

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

    Addr->buf_len = WasmEdge::EndianValue(4).le();

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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_MAX);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{FdServer, AddrPtr, Port},
        Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const auto Msg1 = "hello, wasmedge in ipv6."sv;
    uint32_t Msg1Len = static_cast<uint32_t>(Msg1.size());
    writeString(MemInst, Msg1, MsgInPtr);

    auto *MsgInPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgInPackPtr);
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    auto *AddrBufSend = MemInst.getPointer<in6_addr *>(AddrBufPtr);
    *AddrBufSend = in6addr_loopback;
    Addr->buf_len =
        WasmEdge::EndianValue(static_cast<uint32_t>(sizeof(*AddrBufSend))).le();

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

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
    EXPECT_NE(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(FdPtr))
            .le(),
        UINT32_MAX);

    int32_t Fd =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdPtr)).le();

    uint32_t OptLevel =
        __wasi_sock_opt_level_t::__WASI_SOCK_OPT_LEVEL_SOL_SOCKET;
    uint32_t OptName = __wasi_sock_opt_so_t::__WASI_SOCK_OPT_SO_TYPE;

    auto ResBuf = MemInst.getSpan<uint8_t>(ResBufPtr, ResMaxLen);
    auto ResBufSz = MemInst.getPointer<uint32_t *>(ResBufSzPtr);
    *ResBufSz = WasmEdge::EndianValue(ResMaxLen).le();
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
    EXPECT_NE(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(FdPtr))
            .le(),
        UINT32_MAX);

    int32_t Fd =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdPtr)).le();

    const uint32_t OptLevel = __WASI_SOCK_OPT_LEVEL_SOL_SOCKET;
    const uint32_t OptName = __WASI_SOCK_OPT_SO_BROADCAST;

    auto ResBuf = MemInst.getPointer<decltype(&Opt)>(ResBufPtr);
    auto ResBufSz = MemInst.getPointer<uint32_t *>(ResBufSzPtr);
    *ResBufSz = WasmEdge::EndianValue(ResMaxLen).le();
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
    EXPECT_NE(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(FdPtr))
            .le(),
        UINT32_MAX);

    int32_t Fd =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdPtr)).le();

    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET4))
            .le();
    unalignedCopy(MemInst.getPointer<uint32_t *>(AddrBufPtr + 2), BindAddress);

    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    *MemInst.getPointer<uint32_t *>(ResPortPtr) = UINT32_C(0);

    auto *ResAddr = MemInst.getPointer<__wasi_address_t *>(ResAddrPtr);

    auto ResAddrBuf = MemInst.getSpan<uint8_t>(ResAddrBufPtr, AddrBuflen);
    std::fill_n(ResAddrBuf.data(), ResAddrBuf.size(), 0x00);

    ResAddr->buf = WasmEdge::EndianValue(ResAddrBufPtr).le();
    ResAddr->buf_len = WasmEdge::EndianValue(128).le();

    WasiSockGetLocalAddr.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{Fd, ResAddrPtr, ResPortPtr}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint16_t *>(ResAddrBufPtr),
              WasmEdge::EndianValue(
                  static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET4))
                  .le());
    EXPECT_EQ(unalignedRead(MemInst.getPointer<uint32_t *>(ResAddrBufPtr + 2)),
              BindAddress);
    EXPECT_EQ(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(ResPortPtr))
            .le(),
        Port);

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
    EXPECT_NE(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(FdPtr))
            .le(),
        UINT32_MAX);

    int32_t Fd =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdPtr)).le();

    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET6))
            .le();
    auto *AddrBuf = MemInst.getPointer<in6_addr *>(AddrBufPtr + 2);
    *AddrBuf = in6addr_loopback;

    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

    WasiSockBind.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{Fd, AddrPtr, Port},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    *MemInst.getPointer<uint32_t *>(ResPortPtr) = UINT32_C(0);

    auto *ResAddr = MemInst.getPointer<__wasi_address_t *>(ResAddrPtr);

    auto ResAddrBuf = MemInst.getSpan<uint8_t>(ResAddrBufPtr, AddrBuflen);
    std::fill_n(ResAddrBuf.data(), ResAddrBuf.size(), 0x00);

    ResAddr->buf = WasmEdge::EndianValue(ResAddrBufPtr).le();
    ResAddr->buf_len = WasmEdge::EndianValue(128U).le();

    WasiSockGetLocalAddr.run(
        CallFrame,
        std::array<WasmEdge::ValVariant, 3>{Fd, ResAddrPtr, ResPortPtr}, Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    EXPECT_EQ(*MemInst.getPointer<uint16_t *>(ResAddrBufPtr),
              WasmEdge::EndianValue(
                  static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_INET6))
                  .le());
    const auto LHS =
        MemInst.getSpan<const char>(ResAddrBufPtr + 2, sizeof(in6_addr));
    const auto RHS = WasmEdge::Span<const char>(
        reinterpret_cast<const char *>(&in6addr_loopback), sizeof(in6_addr));
    for (uint32_t I = 0; I < sizeof(in6_addr); ++I) {
      EXPECT_EQ(LHS[I], RHS[I]);
    }
    EXPECT_EQ(
        WasmEdge::EndianValue(*MemInst.getPointer<const uint32_t *>(ResPortPtr))
            .le(),
        Port);

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
  // MaxLength is too large.
  {
    EXPECT_TRUE(WasiGetAddrinfo.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            NodePtr, NodeLen, ServicePtr, ServiceLen, HintsPtr, ResultPtr,
            WasmEdge::Host::WASI::kAddrinfoMax + 1, ResLengthPtr},
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
  // A lookup without a socket type returns all representable results.
  {
    std::string TmpNode = "127.0.0.1";
    writeString(MemInst, TmpNode, NodePtr);
    uint32_t TmpNodeLen = static_cast<uint32_t>(TmpNode.size());
    Hints.ai_flags = __WASI_AIFLAGS_AI_NUMERICHOST;
    Hints.ai_socktype = __WASI_SOCK_TYPE_SOCK_ANY;
    Hints.ai_protocol = __WASI_PROTOCOL_IPPROTO_IP;
    writeAddrinfo(MemInst, &Hints, HintsPtr);
    EXPECT_TRUE(
        WasiGetAddrinfo.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                NodePtr, TmpNodeLen, ServicePtr, ServiceLen,
                                HintsPtr, ResultPtr, MaxLength, ResLengthPtr},
                            Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_GT(*ResLength, 0U);
    auto *ResItem = MemInst.getPointer<__wasi_addrinfo_t *>(*Result);
    for (uint32_t Idx = 0; Idx < *ResLength; ++Idx) {
      EXPECT_TRUE(ResItem->ai_socktype == __WASI_SOCK_TYPE_SOCK_ANY ||
                  ResItem->ai_socktype == __WASI_SOCK_TYPE_SOCK_STREAM ||
                  ResItem->ai_socktype == __WASI_SOCK_TYPE_SOCK_DGRAM);
      if (Idx + 1 < *ResLength) {
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
    std::string TmpNode = "127.0.0.1";
    writeString(MemInst, TmpNode, NodePtr);
    uint32_t TmpNodeLen = static_cast<uint32_t>(TmpNode.size());
    Hints.ai_flags =
        __WASI_AIFLAGS_AI_CANONNAME | __WASI_AIFLAGS_AI_NUMERICHOST;
    Hints.ai_socktype = __WASI_SOCK_TYPE_SOCK_DGRAM;
    Hints.ai_protocol = __WASI_PROTOCOL_IPPROTO_UDP;
    writeAddrinfo(MemInst, &Hints, HintsPtr);
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
#if !WASMEDGE_OS_WINDOWS && !WASMEDGE_OS_MACOS
    EXPECT_NE(ResHead->ai_canonname_len, 0);
    EXPECT_STREQ(MemInst
                     .getSpan<const char>(ResHead->ai_canonname,
                                          ResHead->ai_canonname_len + 1)
                     .data(),
                 "127.0.0.1");
#endif
    auto *WasiSockAddr =
        MemInst.getPointer<__wasi_sockaddr_t *>(ResHead->ai_addr);
    EXPECT_EQ(WasiSockAddr->sa_data_len, 14);
  }
}

TEST(WasiSockTest, GetAddrinfoRejectsShortSocketAddressBuffer) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  WasmEdge::Host::WasiSockGetAddrinfo WasiGetAddrinfo(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  auto Initial =
      prepareAddrinfoTest(MemInst, 1, 0, __WASI_AIFLAGS_AI_NUMERICHOST);
  Initial.Sockaddr.sa_data =
      WasmEdge::Runtime::Instance::MemoryInstance::kPageSize - 1;
  std::memcpy(
      MemInst.getPointer<__wasi_sockaddr_t *>(AddrinfoTestData::SockaddrPtr),
      &Initial.Sockaddr, sizeof(Initial.Sockaddr));
  constexpr uint8_t Canary = 0xa5;
  *MemInst.getPointer<uint8_t *>(Initial.Sockaddr.sa_data) = Canary;

  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiGetAddrinfo.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AddrinfoTestData::NodePtr, 9, AddrinfoTestData::ServicePtr, 2,
          AddrinfoTestData::HintsPtr, AddrinfoTestData::ResultPtr, 1,
          AddrinfoTestData::ResultLengthPtr},
      Errno));

  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  expectAddrinfoFieldsEqual(*MemInst.getPointer<const __wasi_addrinfo_t *>(
                                AddrinfoTestData::AddrinfoPtr),
                            Initial.Addrinfo);
  expectSockaddrFieldsEqual(*MemInst.getPointer<const __wasi_sockaddr_t *>(
                                AddrinfoTestData::SockaddrPtr),
                            Initial.Sockaddr);
  EXPECT_EQ(*MemInst.getPointer<const uint8_t *>(Initial.Sockaddr.sa_data),
            Canary);
  EXPECT_EQ(
      *MemInst.getPointer<const uint32_t *>(AddrinfoTestData::ResultLengthPtr),
      AddrinfoTestData::InitialResultLength);
}

TEST(WasiSockTest, GetAddrinfoRejectsShortIPv6SocketAddressBuffer) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  WasmEdge::Host::WasiSockGetAddrinfo WasiGetAddrinfo(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  const auto Initial = prepareAddrinfoTest(
      MemInst, WasmEdge::Host::WASI::kMaxSaDataLen - 1, 0,
      __WASI_AIFLAGS_AI_NUMERICHOST, "::1", __WASI_ADDRESS_FAMILY_INET6);
  constexpr uint8_t Canary = 0xa5;
  std::fill_n(MemInst.getPointer<uint8_t *>(AddrinfoTestData::SocketDataPtr),
              WasmEdge::Host::WASI::kMaxSaDataLen, Canary);

  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiGetAddrinfo.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AddrinfoTestData::NodePtr, 3, AddrinfoTestData::ServicePtr, 2,
          AddrinfoTestData::HintsPtr, AddrinfoTestData::ResultPtr, 1,
          AddrinfoTestData::ResultLengthPtr},
      Errno));

  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  expectAddrinfoFieldsEqual(*MemInst.getPointer<const __wasi_addrinfo_t *>(
                                AddrinfoTestData::AddrinfoPtr),
                            Initial.Addrinfo);
  expectSockaddrFieldsEqual(*MemInst.getPointer<const __wasi_sockaddr_t *>(
                                AddrinfoTestData::SockaddrPtr),
                            Initial.Sockaddr);
  const auto SocketData = MemInst.getSpan<const uint8_t>(
      AddrinfoTestData::SocketDataPtr, WasmEdge::Host::WASI::kMaxSaDataLen);
  EXPECT_TRUE(std::all_of(SocketData.begin(), SocketData.end(),
                          [](uint8_t Byte) { return Byte == UINT8_C(0xa5); }));
  EXPECT_EQ(
      *MemInst.getPointer<const uint32_t *>(AddrinfoTestData::ResultLengthPtr),
      AddrinfoTestData::InitialResultLength);
}

TEST(WasiSockTest, GetAddrinfoRejectsShortCanonicalNameBuffer) {
#if WASMEDGE_OS_WINDOWS || WASMEDGE_OS_MACOS
  GTEST_SKIP()
      << "Windows and macOS do not return a canonical name for numeric hosts";
#else
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  WasmEdge::Host::WasiSockGetAddrinfo WasiGetAddrinfo(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  const auto Initial = prepareAddrinfoTest(
      MemInst, WasmEdge::Host::WASI::kMaxSaDataLen, 1,
      __WASI_AIFLAGS_AI_NUMERICHOST | __WASI_AIFLAGS_AI_CANONNAME);
  constexpr uint8_t Canary = 0xa5;
  constexpr uint32_t CanarySize = 10;
  std::fill_n(MemInst.getPointer<uint8_t *>(AddrinfoTestData::CanonnamePtr),
              CanarySize + 1, Canary);

  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiGetAddrinfo.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AddrinfoTestData::NodePtr, 9, AddrinfoTestData::ServicePtr, 2,
          AddrinfoTestData::HintsPtr, AddrinfoTestData::ResultPtr, 1,
          AddrinfoTestData::ResultLengthPtr},
      Errno));

  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  expectAddrinfoFieldsEqual(*MemInst.getPointer<const __wasi_addrinfo_t *>(
                                AddrinfoTestData::AddrinfoPtr),
                            Initial.Addrinfo);
  expectSockaddrFieldsEqual(*MemInst.getPointer<const __wasi_sockaddr_t *>(
                                AddrinfoTestData::SockaddrPtr),
                            Initial.Sockaddr);
  const auto Canonname = MemInst.getSpan<const uint8_t>(
      AddrinfoTestData::CanonnamePtr, CanarySize + 1);
  EXPECT_TRUE(std::all_of(Canonname.begin(), Canonname.end(),
                          [](uint8_t Byte) { return Byte == UINT8_C(0xa5); }));
  EXPECT_EQ(
      *MemInst.getPointer<const uint32_t *>(AddrinfoTestData::ResultLengthPtr),
      AddrinfoTestData::InitialResultLength);
#endif
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
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdServerPtr))
                  .le(),
              UINT32_C(-1));
    int32_t FdServer =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdServerPtr))
            .le();

    WasiSockOpen.run(CallFrame,
                     std::array<WasmEdge::ValVariant, 3>{AddressFamily,
                                                         SockType, FdClientPtr},
                     Errno);
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(WasmEdge::EndianValue(
                  *MemInst.getPointer<const uint32_t *>(FdClientPtr))
                  .le(),
              UINT32_C(-1));

    int32_t FdClient =
        WasmEdge::EndianValue(*MemInst.getPointer<const int32_t *>(FdClientPtr))
            .le();

    auto AddrBuf = MemInst.getSpan<uint8_t>(AddrBufPtr, AddrBuflen);
    auto *Addr = MemInst.getPointer<__wasi_address_t *>(AddrPtr);

    std::fill_n(AddrBuf.data(), AddrBuf.size(), 0x00);
    *MemInst.getPointer<uint16_t *>(AddrBufPtr) =
        WasmEdge::EndianValue(
            static_cast<uint16_t>(__WASI_ADDRESS_FAMILY_AF_UNIX))
            .le();

    const std::string Path = "wasmedge_unix_socket_test.sock";
    writeString(MemInst, Path, AddrBufPtr + 2);
    Addr->buf = WasmEdge::EndianValue(AddrBufPtr).le();
    Addr->buf_len = WasmEdge::EndianValue(AddrBuflen).le();

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
    MsgInPack->buf = WasmEdge::EndianValue(MsgInPtr).le();
    MsgInPack->buf_len = WasmEdge::EndianValue(Msg1Len).le();

    WasiSockSendTo.run(CallFrame,
                       std::array<WasmEdge::ValVariant, 7>{
                           FdClient, MsgInPackPtr, UINT32_C(1), AddrPtr, Port,
                           UINT32_C(0), SendtoRetPtr},
                       Errno);

    const auto SendErrno = Errno[0].get<int32_t>();
    if (SendErrno == __WASI_ERRNO_ACCES || SendErrno == __WASI_ERRNO_PERM)
      GTEST_SKIP() << "sock_send_to blocked with WASI errno=" << SendErrno;
    ASSERT_EQ(SendErrno, __WASI_ERRNO_SUCCESS);
    uint32_t MaxMsgBufLen = 100;
    auto MsgBuf = MemInst.getSpan<char>(MsgOutPtr, MaxMsgBufLen);
    std::fill_n(MsgBuf.data(), MsgBuf.size(), 0x00);

    auto *MsgOutPack = MemInst.getPointer<__wasi_ciovec_t *>(MsgOutPackPtr);
    MsgOutPack->buf = WasmEdge::EndianValue(MsgOutPtr).le();
    MsgOutPack->buf_len = WasmEdge::EndianValue(MaxMsgBufLen).le();

    Addr->buf_len = WasmEdge::EndianValue(128).le();

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
