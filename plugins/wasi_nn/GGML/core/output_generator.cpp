// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ggml_core.h"

#include <algorithm>

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

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}"sv, Index)

  // Use index 1 for output metadata.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    uint32_t BytesToCopy =
        std::min(static_cast<uint32_t>(Metadata.length()),
                 static_cast<uint32_t>(OutBuffer.size()));
    std::copy_n(Metadata.data(), BytesToCopy, OutBuffer.data());
    BytesWritten = BytesToCopy;
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutputSingle: with Index {} a.k.a Metadata...Done"sv, Index)
    return ErrNo::Success;
  }

  std::string LastToken = common_token_to_piece(
      GraphRef.LlamaContext.get(), CxtRef.LlamaOutputTokens.back());
  uint32_t BytesToCopy = std::min(static_cast<uint32_t>(LastToken.length()),
                                   static_cast<uint32_t>(OutBuffer.size()));
  std::copy_n(LastToken.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = EndianValue(BytesToCopy).le();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}...Done"sv,
            Index)
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}"sv, Index)

  // Use index 1 for output metadata.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    uint32_t BytesToCopy =
        std::min(static_cast<uint32_t>(Metadata.length()),
                 static_cast<uint32_t>(OutBuffer.size()));
    std::copy_n(Metadata.data(), BytesToCopy, OutBuffer.data());
    BytesWritten = BytesToCopy;
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutput: with Index {} a.k.a Metadata ...Done"sv, Index)
    return ErrNo::Success;
  }

  uint32_t BytesToCopy =
      std::min(static_cast<uint32_t>(CxtRef.LlamaOutputs.size()),
               static_cast<uint32_t>(OutBuffer.size()));
  std::copy_n(CxtRef.LlamaOutputs.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = EndianValue(BytesToCopy).le();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}...Done"sv, Index)
  return ErrNo::Success;
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
