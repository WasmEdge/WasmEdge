// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/base.h"
#include "host/preview2/wasi-sockets/variant_helper.h"
#include "host/wasi/wasibase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace UDP {
// Wasi-Socket

class CreateUDPSocket : public WasiSockets<CreateUDPSocket> {
public:
  CreateUDPSocket(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t AddressFamily,
                    uint32_t RetBufPtr);
};

class StartBind : public WasiSockets<StartBind> {
public:
  StartBind(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t NetHandle, __VARG_12_UINTS, uint32_t RetBufPtr);
};

class FinishBind : public WasiSockets<FinishBind> {
public:
  FinishBind(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class Stream : public WasiSockets<Stream> {
public:
  Stream(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    __VARG_12_UINTS, uint32_t RetBufPtr);
};

class LocalAddress : public WasiSockets<LocalAddress> {
public:
  LocalAddress(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class RemoteAddress : public WasiSockets<RemoteAddress> {
public:
  RemoteAddress(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class AddressFamily : public WasiSockets<AddressFamily> {
public:
  AddressFamily(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

// https://github.com/WebAssembly/wasi-sockets/pull/93
// IPv6 Only function deleted, but maybe turn back...
class IPv6Only : public WasiSockets<IPv6Only> {
public:
  IPv6Only(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetIPv6Only : public WasiSockets<SetIPv6Only> {
public:
  SetIPv6Only(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t Val, uint32_t RetBufPtr);
};

class UnicastHopLimit : public WasiSockets<UnicastHopLimit> {
public:
  UnicastHopLimit(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetUnicastHopLimit : public WasiSockets<SetUnicastHopLimit> {
public:
  SetUnicastHopLimit(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t Val, uint32_t RetBufPtr);
};

class GetReceiveBufferSize : public WasiSockets<GetReceiveBufferSize> {
public:
  GetReceiveBufferSize(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetReceiveBufferSize : public WasiSockets<SetReceiveBufferSize> {
public:
  SetReceiveBufferSize(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t Val, uint32_t RetBufPtr);
};

class GetSendBufferSize : public WasiSockets<GetSendBufferSize> {
public:
  GetSendBufferSize(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetSendBufferSize : public WasiSockets<SetSendBufferSize> {
public:
  SetSendBufferSize(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t Val, uint32_t RetBufPtr);
};

class UDPSocketsSubscribe : public WasiSockets<UDPSocketsSubscribe> {
public:
  UDPSocketsSubscribe(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

class UDPSocketsDrop : public WasiSockets<UDPSocketsDrop> {
public:
  UDPSocketsDrop(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

class IncomingDatagramStreamReceive
    : public WasiSockets<IncomingDatagramStreamReceive> {
public:
  IncomingDatagramStreamReceive(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t MaxResults, uint32_t RetBufPtr);
};

// subscribe: func() -> pollable;
//  only for preview2 and remove for preview3

} // namespace UDP
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
