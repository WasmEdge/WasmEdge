// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED
#include <Python.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::NeuralSpeed {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED
struct Graph {
  bool EnableDebugLog = true;
  std::string ModelType = "llama";
  inline static int GraphNumber = 0;
  Graph() noexcept { Py_Initialize(); }
  ~Graph() noexcept {
    if (Py_IsInitialized()) {
      Py_XDECREF(Model);
      Py_XDECREF(ModelClass);
      Py_XDECREF(NeuralSpeedModule);
    }
  }
  PyObject *Model = nullptr;
  PyObject *NeuralSpeedModule = nullptr;
  PyObject *ModelClass = nullptr;
  int64_t LoadTime = 0;
  int64_t ComputeTime = 0;
};
struct Context {
  Context(size_t Gid, Graph &) noexcept : GraphId(Gid) {}
  size_t GraphId;
  std::vector<long long int> Inputs;
  std::vector<long long int> Outputs;
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
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept;

} // namespace WasmEdge::Host::WASINN::NeuralSpeed
