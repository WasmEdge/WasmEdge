// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
#include "openvino/openvino.hpp"
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::OpenVINO {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
struct Graph {
  ~Graph() noexcept {}
  ov::Tensor OpenVINOIWeightTensor;
  std::shared_ptr<ov::Model> OpenVINOModel;
  Device TargetDevice = Device::AUTO;
};

struct Context {
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  ~Context() noexcept {}
  uint32_t GraphId;
  ov::InferRequest OpenVINOInferRequest;
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
} // namespace WasmEdge::Host::WASINN::OpenVINO
