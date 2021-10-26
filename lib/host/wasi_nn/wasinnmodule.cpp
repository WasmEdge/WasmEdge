#include "host/wasi_nn/wasinnmodule.h"
#include "common/errcode.h"
#include "host/wasi_nn/wasinnfunc.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include <memory>

namespace WasmEdge {
namespace Host {

WasiNNModule::WasiNNModule() : ImportObject("wasi_ephemeral_nn") {
  addHostFunc("load", std::make_unique<WasiNNLoad>(Ctx));
  addHostFunc("init_execution_context",
              std::make_unique<WasiNNInitExecCtx>(Ctx));
  addHostFunc("set_input", std::make_unique<WasiNNSetInput>(Ctx));
  addHostFunc("get_output", std::make_unique<WasiNNGetOuput>(Ctx));
  addHostFunc("compute", std::make_unique<WasiNNCompute>(Ctx));
}

} // namespace Host
} // namespace WasmEdge
