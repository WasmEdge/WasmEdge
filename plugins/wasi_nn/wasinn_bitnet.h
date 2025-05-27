// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "common/errcode.h"
#include "common/span.h"

#include <cstdint>

namespace WasmEdge::Host::WASINN::BitNet {

struct Environ {};

struct Graph {
  void *LlamaModel = nullptr;
  void *LlamaContext = nullptr;
  std::string ModelPath;
  bool EnableLog = false;
};

struct Context {
public:
  Context(size_t GraphId, Graph &) noexcept : GraphId(GraphId) {}
  size_t GraphId;
  std::vector<int> LlamaInputs;
  std::string LlamaOutputs;
  std::string InputText;
  uint32_t CtxSize = 512;
  uint32_t TempSize = 1024;
  bool EnableDebug = false;
};

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept;
Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept;
Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept;
Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept;
Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept;
Expect<ErrNo> computeSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept;
Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept;
Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept;
Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept;

} // namespace WasmEdge::Host::WASINN::BitNet