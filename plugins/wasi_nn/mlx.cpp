#include "mlx.h"
#include "wasinnenv.h"

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &GraphId) noexcept {
  // Add a new graph.
  // Env.NNGraph.emplace_back(Backend::MLX);
  // auto &GraphRef = Env.NNGraph.back().get<Graph>();
  
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t Index,
                               const TensorData &Tensor) noexcept {
  // auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                                uint32_t Index, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  // auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  // auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] MLX backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"MLX\" to build it."sv);
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
                                  uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                                Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
#endif

} // namespace WasmEdge::Host::WASINN::MLX