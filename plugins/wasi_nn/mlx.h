// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include <mlx/mlx.h>
#endif
namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}
namespace WasmEdge::Host::WASINN::MLX {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
struct Graph {
  mlx::core::nn::Module MLXModel;
  mlx::core StreamOrDevice MLXDevice =
      metal::is_available() ? Device::gpu : Device::cpu;
  ;
};

struct Context {
public:
  Context(size_t GId, Graph &) noexcept : GraphId(GId) {}
  size_t GraphId;
  mlx::core::StreamOrDevice MLXDevice =
      metal::is_available() ? Device::gpu : Device::cpu;
  mlx::core::SafeTensorLoad Model;
  std::vector<mlx::core::array> MLXInputs;
  std::vector<mlx::core::array> MLXOutputs;
};

#else
struct Graph {};
struct Context {
  Context(size_t, Graph &) noexcept {}
};
#endif

struct Environ {};

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
} // namespace WasmEdge::Host::WASINN::MLX
