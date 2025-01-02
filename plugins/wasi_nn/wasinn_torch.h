// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
#include <torch/csrc/inductor/aoti_runner/model_container_runner.h>
#include <torch/csrc/inductor/aoti_runner/model_container_runner_cpu.h>
#ifdef TORCHAOTI_USE_CUDA
#include <torch/csrc/inductor/aoti_runner/model_container_runner_cuda.h>
#endif
#include <torch/script.h>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::PyTorch {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH

class PyBaseModule {
public:
  virtual ~PyBaseModule() = default;
  virtual Expect<ErrNo> setDevice(Device Device) = 0;
  virtual Expect<ErrNo> loadFromPath(const std::string &Path,
                                     Device Device) = 0;
  virtual Expect<ErrNo> loadFromBinary(std::istream &In, Device Device) = 0;
  virtual Expect<ErrNo> run(std::vector<at::Tensor> In,
                            std::vector<at::Tensor> &Out) = 0;

  torch::DeviceType getDevice() const { return TorchDevice; }

protected:
  torch::DeviceType TorchDevice = at::kCPU;
};

class TorchScript : public PyBaseModule {
  Expect<ErrNo> setDevice(Device Device) override;

public:
  Expect<ErrNo> loadFromPath(const std::string &Path, Device Device) override;
  Expect<ErrNo> loadFromBinary(std::istream &In, Device Device) override;
  Expect<ErrNo> run(std::vector<at::Tensor> In,
                    std::vector<at::Tensor> &Out) override;

  torch::jit::Module TorchModel;
};

class AOTInductor : public PyBaseModule {
  Expect<ErrNo> setDevice(Device Device) override;

public:
  AOTInductor();
  Expect<ErrNo> loadFromPath(const std::string &Path, Device Device) override;
  Expect<ErrNo> loadFromBinary(std::istream &In, Device Device) override;
  Expect<ErrNo> run(std::vector<at::Tensor> In,
                    std::vector<at::Tensor> &Out) override;

  torch::inductor::AOTIModelContainerRunner *TorchModel;

  ~AOTInductor() {
    if (TorchModel) {
      delete TorchModel;
    }
  }
};

enum class PyModelBackend { TorchScript, AOTInductor, UNKNOWN };

struct Graph {
  PyBaseModule *Model = nullptr;
};

struct Context {
public:
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  uint32_t GraphId;
  std::vector<at::Tensor> TorchInputs;
  std::vector<at::Tensor> TorchOutputs;
};
#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
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
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept;
} // namespace WasmEdge::Host::WASINN::PyTorch
