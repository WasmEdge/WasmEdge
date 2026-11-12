// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
class Graph;
class Context;
} // namespace WasmEdge::Host::WASINN

namespace WasmEdge::Host::WASINN::ONNX {
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};

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
} // namespace WasmEdge::Host::WASINN::ONNX
