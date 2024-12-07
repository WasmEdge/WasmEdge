// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "wasinntypes.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TENSORRT
#include "tensorrt_llm/executor/executor.h"
#include <memory>
#include <optional>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::TensorRT {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TENSORRT
class Tokenizer {
public:
  std::vector<std::string> Id2Token;
  std::unordered_map<std::string_view, uint32_t> Token2Id;
  std::unordered_map<uint32_t, std::string> Id2SpecialToken;
  std::unordered_map<std::string_view, uint32_t> SpecialToken2Id;
  int32_t BOSId = 1;
  int32_t EOSId = 2;
  int32_t UNKId = 0;
  int32_t SEPId = -1;
  int32_t PADId = -1;
  int32_t CLSId = -1;
  int32_t MASKId = -1;
  int32_t LineFeedId = 13;
  int32_t PrefixId = -1;
  int32_t SuffixId = -1;
  int32_t MiddleId = -1;
  int32_t EOTId = -1;

  Expect<WASINN::ErrNo> load(const std::string &JsonFile) noexcept;
  std::vector<tensorrt_llm::executor::TokenIdType>
  parse(Span<const uint8_t> String) noexcept;
  std::string
  unparse(Span<const tensorrt_llm::executor::TokenIdType> Tokens) noexcept;

  static std::array<std::string, 256> Byte2U8Char;
  static std::unordered_map<std::string_view, char> U8Char2Byte;
  static void init() noexcept;
};

struct Graph {
  std::unique_ptr<tensorrt_llm::executor::Executor> Executor;
  tensorrt_llm::executor::ExecutorConfig ExecutorConfig;
  int32_t MaxNewTokens = 1024;
  Tokenizer Tok;
};
struct Context {
  Context(size_t GId, Graph &) noexcept : GraphId(GId) {}
  size_t GraphId;
  std::optional<tensorrt_llm::executor::IdType> RequestId;
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
} // namespace WasmEdge::Host::WASINN::TensorRT
