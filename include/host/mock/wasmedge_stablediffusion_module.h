#pragma once

#include "host/mock/wasmedge_stablediffusion_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
class WasmEdgeStableDiffusionModuleMock
    : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeStableDiffusionModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_stablediffusion") {
    addHostFunc("create_context",
                std::make_unique<WasmEdgeStableDiffusionMock::CreateContext>());
    addHostFunc("text_to_image",
                std::make_unique<WasmEdgeStableDiffusionMock::TextToImage>());
    addHostFunc("image_to_image",
                std::make_unique<WasmEdgeStableDiffusionMock::ImageToImage>());
    addHostFunc("convert",
                std::make_unique<WasmEdgeStableDiffusionMock::Convert>());
  }
};
} // namespace Host
} // namespace WasmEdge
