#pragma once

#include "host/mock/wasi_logging_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

using namespace std::literals;

class WasiLoggingModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiLoggingModuleMock()
      : Runtime::Instance::ModuleInstance("wasi:logging/logging") {
    addHostFunc("log"sv, std::make_unique<WasiLoggingMock::Log>());
  }
};

} // namespace Host
} // namespace WasmEdge
