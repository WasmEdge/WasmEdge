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
namespace TCP {
// Wasi-Socket

class CreateTCPSocket : public WasiSockets<CreateTCPSocket> {
public:
  CreateTCPSocket(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

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

class StartConnect : public WasiSockets<StartConnect> {
public:
  StartConnect(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t NetHandle, __VARG_12_UINTS, uint32_t RetBufPtr);
};

class FinishConnect : public WasiSockets<FinishConnect> {
public:
  FinishConnect(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class StartListen : public WasiSockets<StartListen> {
public:
  StartListen(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class FinishListen : public WasiSockets<FinishListen> {
public:
  FinishListen(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class Accept : public WasiSockets<Accept> {
public:
  Accept(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
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

class Islistening : public WasiSockets<Islistening> {
public:
  Islistening(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

class AddressFamily : public WasiSockets<AddressFamily> {
public:
  AddressFamily(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

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

class SetListenBacklogSize : public WasiSockets<SetListenBacklogSize> {
public:
  SetListenBacklogSize(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t Val, uint32_t RetBufPtr);
};

class KeepAliveEnabled : public WasiSockets<KeepAliveEnabled> {
public:
  KeepAliveEnabled(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetKeepAliveEnabled : public WasiSockets<SetKeepAliveEnabled> {
public:
  SetKeepAliveEnabled(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t Val, uint32_t RetBufPtr);
};

class KeepAliveIdleTime : public WasiSockets<KeepAliveIdleTime> {
public:
  KeepAliveIdleTime(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetKeepAliveIdleTime : public WasiSockets<SetKeepAliveIdleTime> {
public:
  SetKeepAliveIdleTime(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t Val, uint32_t RetBufPtr);
};

class KeepAliveInterval : public WasiSockets<KeepAliveInterval> {
public:
  KeepAliveInterval(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetKeepAliveInterval : public WasiSockets<SetKeepAliveInterval> {
public:
  SetKeepAliveInterval(WasiSocketsEnvironment &HostEnv)
      : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint64_t Val, uint32_t RetBufPtr);
};

class KeepAliveCount : public WasiSockets<KeepAliveCount> {
public:
  KeepAliveCount(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetKeepAliveCount : public WasiSockets<SetKeepAliveCount> {
public:
  SetKeepAliveCount(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t Val, uint32_t RetBufPtr);
};

class HopLimit : public WasiSockets<HopLimit> {
public:
  HopLimit(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class SetHopLimit : public WasiSockets<SetHopLimit> {
public:
  SetHopLimit(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

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

class Shutdown : public WasiSockets<Shutdown> {
public:
  Shutdown(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t How, uint32_t RetBufPtr);
};
} // namespace TCP
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
