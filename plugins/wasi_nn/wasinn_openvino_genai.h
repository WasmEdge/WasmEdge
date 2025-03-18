// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINOGENAI
#include "openvino/openvino.hpp"
#include <openvino/genai/llm_pipeline.hpp>
#include <openvino/genai/visual_language/pipeline.hpp>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::OpenVINOGenAI {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINOGENAI

struct Context;
class OpenVINOGenAIBackend {
public:
  OpenVINOGenAIBackend() = default;
  virtual Expect<WASINN::ErrNo> SetContextInput(Context &CxtRef, uint32_t Index,
                                                const TensorData &Tensor) = 0;
  virtual Expect<WASINN::ErrNo> Generate(Context &CxtRef) = 0;
  virtual Expect<WASINN::ErrNo> GetContextOutput(Context &CxtRef,
                                                 uint32_t Index,
                                                 Span<uint8_t> OutBuffer,
                                                 uint32_t &BytesWritten) = 0;
  virtual ~OpenVINOGenAIBackend() noexcept {}
};

class LLMPipelineBackend : public OpenVINOGenAIBackend {
public:
  LLMPipelineBackend(std::string Path, std::string Device) {
    Model = std::make_shared<ov::genai::LLMPipeline>(Path, Device);
  }
  ~LLMPipelineBackend() noexcept {}
  Expect<WASINN::ErrNo> SetContextInput(Context &CxtRef, uint32_t Index,
                                        const TensorData &Tensor) override;
  Expect<WASINN::ErrNo> Generate(Context &CxtRef) override;
  Expect<WASINN::ErrNo> GetContextOutput(Context &CxtRef, uint32_t Index,
                                         Span<uint8_t> OutBuffer,
                                         uint32_t &BytesWritten) override;

private:
  std::shared_ptr<ov::genai::LLMPipeline> Model;
};

struct Graph {
  ~Graph() noexcept {}
  std::shared_ptr<OpenVINOGenAIBackend> OpenVINOGenAI;
  Device TargetDevice = Device::AUTO;
};

struct Context {
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  ~Context() noexcept {}
  uint32_t GraphId;
  std::string StringInput;
  std::string StringOutput;

  // For image input/output
  // ov::Tensor TensorInput;
  // ov::Tensor TensorOutput;
};

struct Environ {
  Environ() noexcept {}
  ~Environ() noexcept {}
  ov::Core OpenVINOCore;
};
#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
struct Environ {};
#endif

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept;
} // namespace WasmEdge::Host::WASINN::OpenVINOGenAI
