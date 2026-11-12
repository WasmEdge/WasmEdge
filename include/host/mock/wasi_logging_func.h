#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasiLoggingMock {

using namespace std::literals;

class Log : public Runtime::HostFunction<Log> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t, uint32_t,
                    uint32_t, uint32_t) {
    printPluginMock("wasi-logging"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

} // namespace WasiLoggingMock
} // namespace Host
} // namespace WasmEdge
