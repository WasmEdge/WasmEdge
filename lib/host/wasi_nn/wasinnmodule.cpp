#include "host/wasi_nn/wasinnmodule.h"
#include "common/errcode.h"
#include "host/wasi_nn/wasinnfunc.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>
#include <onnxruntime_cxx_api.h>

namespace WasmEdge {
namespace Host {

WasiNNModule::WasiNNModule() : ImportObject("wasi_ephemeral_nn") {
  assert(sizeof(float) == 4);
  spdlog::set_level(spdlog::level::info);
  spdlog::stdout_color_mt("WasiNN");

  this->Ctx.MemoryInfo =
      std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(
          OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));

  addHostFunc("load", std::make_unique<WasiNNLoad>(Ctx));
  addHostFunc("init_execution_context",
              std::make_unique<WasiNNInitExecCtx>(Ctx));
  addHostFunc("set_input", std::make_unique<WasiNNSetInput>(Ctx));
  addHostFunc("get_output", std::make_unique<WasiNNGetOuput>(Ctx));
  addHostFunc("compute", std::make_unique<WasiNNCompute>(Ctx));
}

} // namespace Host
} // namespace WasmEdge
