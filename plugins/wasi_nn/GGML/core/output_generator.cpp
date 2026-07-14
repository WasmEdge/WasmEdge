// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ggml_core.h"

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
namespace {
// Generate output metadata.
std::string buildOutputMetadata(Context &CxtRef) noexcept {
  return fmt::format(R"({{"input_tokens": {}, )"
                     R"("output_tokens": {}, )"
                     R"("llama_build_number": {}, )"
                     R"("llama_commit": "{}"}})"sv,
                     CxtRef.LlamaNInputs, CxtRef.LlamaOutputTokens.size(),
                     LLAMA_BUILD_NUMBER, LLAMA_COMMIT);
}
} // namespace

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &, WASINN::Graph &G,
                              WASINN::Context &C, uint32_t Index,
                              Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}"sv, Index)

  // Use index 1 for output metadata.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    BytesWritten = EndianValue(static_cast<uint32_t>(Metadata.length())).le();
    if (OutBuffer.size() < Metadata.length()) {
      spdlog::error("[WASI-NN] GGML backend: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    Metadata.length(), OutBuffer.size());
      return ErrNo::TooLarge;
    }
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutputSingle: with Index {} a.k.a Metadata...Done"sv, Index)
    return ErrNo::Success;
  }

  if (CxtRef.LlamaOutputTokens.empty()) {
    BytesWritten = 0;
    return ErrNo::Success;
  }

  // A failed set_input reload can null the llama context; unload then makes
  // the Invalid graph Drainable, so this op can be admitted without one.
  if (GraphRef.LlamaContext == nullptr) {
    RET_ERROR(ErrNo::InvalidArgument,
              "getOutputSingle: the llama context is gone. The last reload "sv
              "failed and the graph was unloaded."sv)
  }

  std::string LastToken = common_token_to_piece(
      GraphRef.LlamaContext.get(), CxtRef.LlamaOutputTokens.back());
  BytesWritten = EndianValue(static_cast<uint32_t>(LastToken.length())).le();
  if (OutBuffer.size() < LastToken.length()) {
    spdlog::error("[WASI-NN] GGML backend: output buffer too small, "
                  "need {} bytes but got {}."sv,
                  LastToken.length(), OutBuffer.size());
    return ErrNo::TooLarge;
  }
  std::copy_n(LastToken.data(), LastToken.length(), OutBuffer.data());
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}...Done"sv,
            Index)
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &, WASINN::Graph &G,
                        WASINN::Context &C, uint32_t Index,
                        Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}"sv, Index)

  // Use index 1 for output metadata.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    BytesWritten = EndianValue(static_cast<uint32_t>(Metadata.length())).le();
    if (OutBuffer.size() < Metadata.length()) {
      spdlog::error("[WASI-NN] GGML backend: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    Metadata.length(), OutBuffer.size());
      return ErrNo::TooLarge;
    }
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutput: with Index {} a.k.a Metadata ...Done"sv, Index)
    return ErrNo::Success;
  }

  BytesWritten =
      EndianValue(static_cast<uint32_t>(CxtRef.LlamaOutputs.size())).le();
  if (OutBuffer.size() < CxtRef.LlamaOutputs.size()) {
    spdlog::error("[WASI-NN] GGML backend: output buffer too small, "
                  "need {} bytes but got {}."sv,
                  CxtRef.LlamaOutputs.size(), OutBuffer.size());
    return ErrNo::TooLarge;
  }
  std::copy_n(CxtRef.LlamaOutputs.data(), CxtRef.LlamaOutputs.size(),
              OutBuffer.data());
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}...Done"sv, Index)
  return ErrNo::Success;
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
