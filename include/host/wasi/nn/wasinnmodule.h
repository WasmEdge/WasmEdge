#pragma once

#include "host/wasi/nn/wasinncontext.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasiNNModule : public Runtime::ImportObject {
public:
  WasiNNModule();
  virtual ~WasiNNModule() = default;

private:
  WasiNNContext Ctx;
};

} // namespace Host
} // namespace WasmEdge
