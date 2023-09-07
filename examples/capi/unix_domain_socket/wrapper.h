#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

__attribute__((import_module("wasi_snapshot_preview1"),
               import_name("sock_open_v2"))) uint32_t
sock_open_v2(uint32_t AddressFamily, uint32_t SockType, uint32_t *Fd);

__attribute__((import_module("wasi_snapshot_preview1"),
               import_name("sock_bind_v2"))) uint32_t
sock_bind_v2(int32_t Fd, void *AddressPtr, uint32_t Port);

__attribute__((import_module("wasi_snapshot_preview1"),
               import_name("sock_listen_v2"))) uint32_t
sock_listen_v2(int32_t Fd, int32_t Backlog);

__attribute__((import_module("wasi_snapshot_preview1"),
               import_name("sock_accept_v2"))) uint32_t
sock_accept_v2(int32_t Fd, uint32_t FsFlags, uint32_t *NFd);

__attribute__((import_module("wasi_snapshot_preview1"),
               import_name("sock_connect_v2"))) uint32_t
sock_connect_v2(int32_t Fd, void *AddressPtr, uint32_t Port);

static uint32_t toWasmEdgeAddressFamily(int Domain) {
  if (Domain == AF_UNIX)
    return 3U; //__WASI_ADDRESS_FAMILY_AF_UNIX
  __builtin_unreachable();
}

static uint32_t toWasmEdgeSockType(int Type) {
  if (Type == SOCK_STREAM)
    return 2U; // __WASI_SOCK_TYPE_SOCK_STREAM
  __builtin_unreachable();
}

struct WasmEdgeAddress {
  void *Buf;
  uint32_t BufLen;
};

struct WasmEdgeAddressInnerV2 {
  char Buf[128];
};

int WasmedgeSocket(int Domain, int Type, int Protocol) {
  uint32_t AddressFamily = toWasmEdgeAddressFamily(Domain);
  uint32_t SockType = toWasmEdgeSockType(Type);
  uint32_t Fd = 0, Err;

  Err = sock_open_v2(AddressFamily, SockType, &Fd);

  if (Err)
    return -1;
  return Fd;
}

int WasmedgeBind(int Fd, const sockaddr *Addr, socklen_t Len) {
  uint32_t Err;
  WasmEdgeAddressInnerV2 AddrV2{};
  WasmEdgeAddress WasmEdgeAddr;

  *reinterpret_cast<short *>(&AddrV2.Buf[0]) =
      toWasmEdgeAddressFamily(Addr->sa_family);
  std::strcpy(&AddrV2.Buf[2], Addr->sa_data);

  WasmEdgeAddr.Buf = AddrV2.Buf;
  WasmEdgeAddr.BufLen = sizeof(AddrV2.Buf);

  Err = sock_bind_v2(Fd, &WasmEdgeAddr, 0);

  if (Err)
    return -1;
  return 0;
}

int WasmedgeListen(int Fd, int N) {
  uint32_t Err;

  Err = sock_listen_v2(Fd, N);

  if (Err)
    return -1;
  return 0;
}

int WasmedgeAccept(int Fd, sockaddr *Addr, socklen_t *Len) {
  uint32_t Err;
  uint32_t NFd;

  uint32_t Flag = 0; // 0 for block mode

  Err = sock_accept_v2(Fd, Flag, &NFd);

  if (Err)
    return -1;
  return NFd;
}

int WasmedgeConnect(int Fd, const sockaddr *Addr, socklen_t Len) {
  uint32_t Err;
  WasmEdgeAddressInnerV2 AddrV2{};
  WasmEdgeAddress WasmEdgeAddr;

  *reinterpret_cast<short *>(&AddrV2.Buf[0]) =
      toWasmEdgeAddressFamily(Addr->sa_family);
  std::strcpy(&AddrV2.Buf[2], Addr->sa_data);

  WasmEdgeAddr.Buf = AddrV2.Buf;
  WasmEdgeAddr.BufLen = sizeof(AddrV2.Buf);

  Err = sock_connect_v2(Fd, &WasmEdgeAddr, 0);

  if (Err)
    return -1;
  return 0;
}
