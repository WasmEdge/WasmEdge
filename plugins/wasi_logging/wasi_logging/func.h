#pragma once

#include "wasi_logging/base.h"

namespace WasmEdge {
namespace Host {

class WasiLoggingLog : public WasiLogging<WasiLoggingLog> {
public:
  WasiLoggingLog(WasiLoggingEnvironment &HostEnv) : WasiLogging(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Level,
                    uint32_t CxtPtr, uint32_t CxtLen, uint32_t MsgPtr,
                    uint32_t MsgLen);
};

} // namespace Host
} // namespace WasmEdge
