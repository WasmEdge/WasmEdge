// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "whispercpp.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
// #include "simdjson.h"
#include "whisper.h"
#include <algorithm>
#include <common.h>
#include <cstdlib>
#include <sstream>
#endif

namespace WasmEdge::Host::WASINN::WHISPER {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

Expect<ErrNo> load([[maybe_unused]] WasiNNEnvironment &Env,
                   [[maybe_unused]] Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device,
                   [[maybe_unused]] uint32_t &GraphId) noexcept {
  // Env.NNGraph.emplace_back(Backend::WHISPER);
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx([[maybe_unused]] WASINN::WasiNNEnvironment &,
                                  [[maybe_unused]] uint32_t,
                                  uint32_t &) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo> setInput([[maybe_unused]] WASINN::WasiNNEnvironment &,
                               [[maybe_unused]] uint32_t,
                               [[maybe_unused]] uint32_t,
                               [[maybe_unused]] const TensorData &) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo> getOutput([[maybe_unused]] WASINN::WasiNNEnvironment &,
                                [[maybe_unused]] uint32_t,
                                [[maybe_unused]] uint32_t,
                                [[maybe_unused]] Span<uint8_t>,
                                [[maybe_unused]] uint32_t &) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo>
getOutputSingle([[maybe_unused]] WASINN::WasiNNEnvironment &Env,
                [[maybe_unused]] uint32_t ContextId,
                [[maybe_unused]] uint32_t Index,
                [[maybe_unused]] Span<uint8_t> OutBuffer,
                [[maybe_unused]] uint32_t &BytesWritten) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo> compute([[maybe_unused]] WASINN::WasiNNEnvironment &,
                              [[maybe_unused]] uint32_t) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo>
computeSingle([[maybe_unused]] WASINN::WasiNNEnvironment &Env,
              [[maybe_unused]] uint32_t ContextId) noexcept {
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo>
finiSingle([[maybe_unused]] WASINN::WasiNNEnvironment &Env,
           [[maybe_unused]] uint32_t ContextId) noexcept {
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] whisper backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"whisper\" to build it."sv);
  return ErrNo::InvalidArgument;
}
} // namespace

Expect<ErrNo> load(WasiNNEnvironment &, Span<const Span<uint8_t>>, Device,
                   uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> initExecCtx(WasiNNEnvironment &, uint32_t, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> setInput(WasiNNEnvironment &, uint32_t, uint32_t,
                       const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutput(WasiNNEnvironment &, uint32_t, uint32_t, Span<uint8_t>,
                        uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> compute(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::WHISPER