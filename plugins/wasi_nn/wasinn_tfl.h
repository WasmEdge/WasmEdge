// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
#include "tensorflow/lite/c/c_api.h"
#include <vector>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
class Graph;
class Context;
} // namespace WasmEdge::Host::WASINN

namespace WasmEdge::Host::WASINN::TensorflowLite {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
struct Graph {
  Graph() noexcept = default;
  Graph(const Graph &) = delete;
  Graph &operator=(const Graph &) = delete;
  ~Graph() noexcept {
    if (TFLiteMod) {
      TfLiteModelDelete(TFLiteMod);
    }
  }
  std::vector<unsigned char> TfLiteModData;
  TfLiteModel *TFLiteMod = nullptr;
};

struct Context {
public:
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  ~Context() noexcept {
    if (TFLiteInterp) {
      TfLiteInterpreterDelete(TFLiteInterp);
    }
  }
  uint32_t GraphId;
  TfLiteInterpreter *TFLiteInterp = nullptr;
};
#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
#endif

struct Environ {};

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  WASINN::Graph &G,
                                  WASINN::Context &C) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                WASINN::Graph &G, WASINN::Context &C,
                                uint32_t Index, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                              WASINN::Context &C) noexcept;
} // namespace WasmEdge::Host::WASINN::TensorflowLite
