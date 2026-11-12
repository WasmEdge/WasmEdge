// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinn_neuralspeed.h"
#include "wasinnenv.h"

namespace WasmEdge::Host::WASINN::NeuralSpeed {
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] Neural Speed backend is removed due to the upstream "
                "end-of-life. Reference: "
                "https://github.com/intel/neural-speed"sv);
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                           Span<const Span<uint8_t>>, WASINN::Device) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                  WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                               WASINN::Context &, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                WASINN::Context &, uint32_t, Span<uint8_t>,
                                uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                              WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
} // namespace WasmEdge::Host::WASINN::NeuralSpeed
