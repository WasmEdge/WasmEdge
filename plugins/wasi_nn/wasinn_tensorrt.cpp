// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_tensorrt.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TENSORRT

#include "simdjson.h"
#include <NvInferRuntime.h>
#include <queue>
#include <tensorrt_llm/executor/version.h>

extern "C" bool initTrtLlmPlugins(void *Logger,
                                  char const *LibNamespace) noexcept;
namespace WasmEdge::Host::WASINN::TensorRT {
namespace {

class LoggerProxy : public nvinfer1::ILogger {
public:
  void log(Severity S, const nvinfer1::AsciiChar *Msg) noexcept override {
    const std::string_view View = Msg;
    switch (S) {
    case Severity::kINTERNAL_ERROR:
      spdlog::critical("{}"sv, View);
      break;
    case Severity::kERROR:
      spdlog::error("{}"sv, View);
      break;
    case Severity::kWARNING:
      spdlog::warn("{}"sv, View);
      break;
    case Severity::kINFO:
      spdlog::info("{}"sv, View);
      break;
    case Severity::kVERBOSE:
      spdlog::debug("{}"sv, View);
      break;
    default:
      spdlog::error("[WASI-NN] TensorRT backend: Unknown TensorRT Severty `{}` "
                    "with message `{}`"sv,
                    static_cast<int32_t>(S), View);
      break;
    }
  }
};

struct PluginLoader {
  PluginLoader() noexcept {
    auto Proxy = std::make_unique<LoggerProxy>();
    if (!initTrtLlmPlugins(Proxy.release(), "tensorrt_llm")) {
      spdlog::error(
          "[WASI-NN] TensorRT backend: Cannot initialize tensorrt_llm plugin."sv);
    }
  }
};

static PluginLoader StaticPluginLoader;

} // namespace

std::array<std::string, 256> Tokenizer::Byte2U8Char;
std::unordered_map<std::string_view, char> Tokenizer::U8Char2Byte;

Expect<WASINN::ErrNo> Tokenizer::load(const std::string &JsonFile) noexcept {
  try {
    simdjson::dom::parser Parser;
    simdjson::dom::object Doc;
    if (Parser.load(JsonFile).get(Doc)) {
      spdlog::error(
          "[WASI-NN] TensorRT backend: Cannot parse tokenizer.json."sv);
      return ErrNo::InvalidArgument;
    }

    simdjson::dom::object Model;
    if (Doc.at_key("model").get(Model) != simdjson::SUCCESS) {
      spdlog::error(
          "[WASI-NN] TensorRT backend: no model in tokenizer.json."sv);
      return ErrNo::InvalidArgument;
    }

    simdjson::dom::object Vocab;
    if (Model.at_key("vocab").get(Vocab) != simdjson::SUCCESS) {
      spdlog::error(
          "[WASI-NN] TensorRT backend: no vocab in tokenizer.json."sv);
      return ErrNo::InvalidArgument;
    }

    Id2Token.resize(Vocab.size());
    for (const auto &Pair : Vocab) {
      uint64_t Value;
      if (Pair.value.get(Value) != simdjson::SUCCESS ||
          Value >= std::numeric_limits<int32_t>::max()) {
        spdlog::error("[WASI-NN] TensorRT backend: id too big in tokenizer."sv);
        return ErrNo::InvalidArgument;
      }
      if (Value <= Id2Token.size()) {
        Id2Token.resize(Value + 1);
      }
      Id2Token[Value] = Pair.key;
      Token2Id[Id2Token[Value]] = Value;
    }

    simdjson::dom::array AddedTokens;
    if (Doc.at_key("added_tokens").get(AddedTokens) != simdjson::SUCCESS) {
      spdlog::error(
          "[WASI-NN] TensorRT backend: no added_tokens in tokenizer.json."sv);
    }
    for (const auto &SpecialToken : AddedTokens) {
      uint64_t Id;
      if (SpecialToken.at_key("id").get(Id) != simdjson::SUCCESS) {
        continue;
      }
      std::string_view Content;
      if (SpecialToken.at_key("content").get(Content) != simdjson::SUCCESS) {
        continue;
      }
      SpecialToken2Id[Id2SpecialToken[Id] = std::string{Content}] = Id;
    }
    EOTId = SpecialToken2Id.find("<|eot_id|>"sv)->second;

    return {};
  } catch (std::bad_alloc &) {
    return ErrNo::InvalidArgument;
  }
}

void Tokenizer::init() noexcept {
  try {
    if (!U8Char2Byte.empty()) {
      return;
    }
    auto Record = [](char C, uint32_t CodePoint) noexcept {
      char String[2];
      if (CodePoint <= 0x7f) {
        String[0] = static_cast<char>(CodePoint);
        Byte2U8Char[C] = std::string{String, 1};
        U8Char2Byte.emplace(std::string_view{Byte2U8Char[C]}, C);
      } else {
        assuming(CodePoint <= 0x7ff);
        String[0] = static_cast<char>((CodePoint >> 6) | 0xc0);
        String[1] = static_cast<char>((CodePoint & 0x3f) | 0x80);
        Byte2U8Char[C] = std::string{String, 2};
        U8Char2Byte.emplace(std::string_view{Byte2U8Char[C]}, C);
      }
    };
    for (uint32_t C = '!'; C <= '~'; ++C) {
      Record(static_cast<char>(C), C);
    }
    for (uint32_t C = 0xa1; C <= 0xac; ++C) {
      Record(static_cast<char>(C), C);
    }
    for (uint32_t C = 0xae; C <= 0xff; ++C) {
      Record(static_cast<char>(C), C);
    }
    uint32_t N = 0;
    for (uint32_t C = 0; C < 256; ++C) {
      if (Byte2U8Char[C].empty()) {
        Record(static_cast<char>(C), 0x100 + N);
        ++N;
      }
    }
  } catch (std::bad_alloc &) {
    return;
  }
}

std::vector<tensorrt_llm::executor::TokenIdType>
Tokenizer::parse(Span<const uint8_t> StringView) noexcept {
  try {
    init();
    std::string NormalizedString;
    NormalizedString.reserve(StringView.size() * 2);
    for (uint8_t C : StringView) {
      NormalizedString.append(Byte2U8Char[C]);
    }
    std::vector<std::tuple<std::string_view, size_t, size_t>> Chars;
    Chars.reserve(NormalizedString.size());
    for (size_t I = 0; I < NormalizedString.size(); ++I) {
      const auto Data =
          reinterpret_cast<const char *>(NormalizedString.data()) + I;
      const size_t Left = Chars.size() - 1;
      const size_t Right = Chars.size() + 1;
      bool IsSpecial = false;
      for (auto &[SpecialToken, Id] : SpecialToken2Id) {
        if (NormalizedString.size() - I >= SpecialToken.size() &&
            SpecialToken == std::string_view{Data, SpecialToken.size()}) {
          Chars.emplace_back(std::string_view{Data, SpecialToken.size()}, Left,
                             Right);
          I += SpecialToken.size() - 1;
          IsSpecial = true;
          break;
        }
      }
      if (!IsSpecial) {
        switch (NormalizedString[I] >> 4) {
        default:
          Chars.emplace_back(std::string_view{Data, 1}, Left, Right);
          I += 0;
          break;
        case 12:
        case 13:
          Chars.emplace_back(std::string_view{Data, 2}, Left, Right);
          I += 1;
          break;
        case 14:
          Chars.emplace_back(std::string_view{Data, 3}, Left, Right);
          I += 2;
          break;
        case 15:
          Chars.emplace_back(std::string_view{Data, 4}, Left, Right);
          I += 3;
          break;
        }
      }
      if (I >= NormalizedString.size()) {
        // missing data at end
        return {};
      }
    }
    size_t Length = Chars.size();
    std::get<1>(Chars.front()) = std::get<2>(Chars.back()) =
        std::numeric_limits<size_t>::max();
    // merge tokens
    std::queue<std::tuple<size_t, size_t, size_t>> Queue;
    for (size_t J = 1; J < Chars.size(); ++J) {
      const size_t I = J - 1;
      auto &[LS, LL, LR] = Chars[I];
      auto &[RS, RL, RR] = Chars[J];
      const size_t Size = LS.size() + RS.size();
      std::string_view CombinedToken{LS.data(), Size};
      if (auto Iter = Token2Id.find(CombinedToken); Iter != Token2Id.end()) {
        Queue.emplace(I, J, Size);
      }
    }
    while (!Queue.empty()) {
      auto [I, J, Size] = Queue.front();
      Queue.pop();
      auto &[LS, LL, LR] = Chars[I];
      auto &[RS, RL, RR] = Chars[J];
      if (LS.empty() || RS.empty() || LS.size() + RS.size() != Size) {
        continue;
      }
      // merge two token
      LS = std::string_view(LS.data(), Size);
      RS = std::string_view();
      LR = RR;
      RL = RR = std::numeric_limits<size_t>::max();
      --Length;
      if (LL != std::numeric_limits<size_t>::max()) {
        const auto S = std::get<0>(Chars[LL]);
        if (!S.empty()) {
          const size_t Size2 = S.size() + Size;
          std::string_view CombinedToken{S.data(), Size2};
          if (auto Iter = Token2Id.find(CombinedToken);
              Iter != Token2Id.end()) {
            Queue.emplace(LL, I, Size2);
          }
        }
      }
      if (LR != std::numeric_limits<size_t>::max()) {
        const auto S = std::get<0>(Chars[LR]);
        if (!S.empty()) {
          const size_t Size2 = Size + S.size();
          std::string_view CombinedToken{LS.data(), Size2};
          if (auto Iter = Token2Id.find(CombinedToken);
              Iter != Token2Id.end()) {
            Queue.emplace(I, LR, Size2);
          }
        }
      }
    }
    std::vector<tensorrt_llm::executor::TokenIdType> Tokens;
    Tokens.reserve(Length);
    for (size_t I = 0; I < Chars.size(); I = std::get<2>(Chars[I])) {
      if (auto Iter = SpecialToken2Id.find(std::get<0>(Chars[I]));
          Iter != SpecialToken2Id.end()) {
        Tokens.push_back(Iter->second);
        continue;
      }
      if (auto Iter = Token2Id.find(std::get<0>(Chars[I]));
          Iter != Token2Id.end()) {
        Tokens.push_back(Iter->second);
      }
    }
    Tokens.shrink_to_fit();
    return Tokens;
  } catch (std::bad_alloc &) {
    return {};
  }
}

std::string Tokenizer::unparse(
    Span<const tensorrt_llm::executor::TokenIdType> Tokens) noexcept {
  try {
    init();
    std::string NormalizedString;
    for (const auto Token : Tokens) {
      if (auto Iter = Id2SpecialToken.find(Token);
          Iter != Id2SpecialToken.end()) {
        NormalizedString += Iter->second;
      } else {
        NormalizedString += Id2Token[Token];
      }
    }
    std::string String;
    String.reserve(NormalizedString.size());
    std::string_view NSView = NormalizedString;
    while (NSView.size() > 0) {
      if (static_cast<uint8_t>(NSView[0]) >= 0x80) {
        String.push_back(U8Char2Byte[NSView.substr(0, 2)]);
        NSView = NSView.substr(2);
      } else {
        String.push_back(U8Char2Byte[NSView.substr(0, 1)]);
        NSView = NSView.substr(1);
      }
    }
    String.shrink_to_fit();

    return String;
  } catch (std::bad_alloc &) {
    return {};
  }
}

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           [[maybe_unused]] Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device,
                           [[maybe_unused]] uint32_t &GraphId) noexcept {
  // Add a new graph.
  try {
    Env.NNGraph.emplace_back(Backend::TensorRT);
    auto &GraphRef = Env.NNGraph.back().get<Graph>();

    if ((Device != WASINN::Device::GPU)) {
      spdlog::error("[WASI-NN] TensorRT Only support GPU target.");
      return WASINN::ErrNo::InvalidArgument;
    }

    if (Builders.size() < 3) {
      spdlog::error("[WASI-NN] TensorRT require three files to run: "
                    "rank0.engine, config.json, and tokenizer.json."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    if (Builders.size() > 3) {
      std::string_view Metadata(
          reinterpret_cast<const char *>(Builders[3].data()),
          Builders[3].size());
      // Ignore context or model updates when initializing the graph.
      simdjson::dom::parser Parser;
      simdjson::dom::object Doc;
      if (Parser.parse(Metadata).get(Doc)) {
        spdlog::error("[WASI-NN] TensorRT backend: Parse metadata error."sv);
        Env.NNGraph.pop_back();
        return ErrNo::InvalidEncoding;
      }

      if (simdjson::dom::element Element;
          Doc.at_key("max-new-tokens").get(Element) == simdjson::SUCCESS) {
        if (int64_t Value;
            Element.get(Value) ||
            Value <
                std::numeric_limits<decltype(GraphRef.MaxNewTokens)>::min() ||
            Value >
                std::numeric_limits<decltype(GraphRef.MaxNewTokens)>::max()) {
          spdlog::error("[WASI-NN] TensorRT backend: Unable to retrieve the "
                        "max-new-tokens option."sv);
          return ErrNo::InvalidArgument;
        } else {
          GraphRef.MaxNewTokens = Value;
        }
      }

#define RetrieveOption(Key, Type, Func)                                        \
  do {                                                                         \
    if (simdjson::dom::element Element;                                        \
        Doc.at_key(Key).get(Element) == simdjson::SUCCESS) {                   \
      if (Type Value; Element.get(Value)) {                                    \
        spdlog::error(                                                         \
            "[WASI-NN] TensorRT backend: Unable to retrieve the " Key          \
            " option."sv);                                                     \
        return ErrNo::InvalidArgument;                                         \
      } else {                                                                 \
        GraphRef.ExecutorConfig.Func(Value);                                   \
      }                                                                        \
    }                                                                          \
  } while (false)

      RetrieveOption("max-beam-width", uint64_t, setMaxBeamWidth);
      RetrieveOption("max-batch-size", uint64_t, setMaxBatchSize);
      RetrieveOption("max-num-tokens", uint64_t, setMaxNumTokens);
      RetrieveOption("enable-chunked-context", bool, setEnableChunkedContext);
      RetrieveOption("normalize-log-probs", bool, setNormalizeLogProbs);
      RetrieveOption("iter-stats-max-iterations", uint64_t,
                     setIterStatsMaxIterations);
      RetrieveOption("request-stats-max-iterations", uint64_t,
                     setRequestStatsMaxIterations);
      RetrieveOption("gpu-weights-percent", double, setGpuWeightsPercent);

#undef RetrieveOption
    }

    // tokenizer.json
    std::string TokenizerJsonFile =
        std::filesystem::u8path(
            std::string_view{reinterpret_cast<const char *>(Builders[2].data()),
                             Builders[2].size()})
            .u8string();

    if (auto Res = GraphRef.Tok.load(TokenizerJsonFile);
        *Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] TensorRT backend: Tokenizer parse error."sv);
      return Res;
    }

    // rank0.engine.
    std::vector<uint8_t> Engine;
    {
      std::ifstream Fs{std::filesystem::u8path(std::string_view{
                           reinterpret_cast<const char *>(Builders[0].data()),
                           Builders[0].size()}),
                       std::ios_base::binary};
      Fs.seekg(0, std::ios::end);
      const size_t Size = static_cast<size_t>(Fs.tellg());
      Fs.seekg(0, std::ios::beg);
      Engine.resize(Size);
      Fs.read(reinterpret_cast<char *>(Engine.data()), Size);
    }

    // config.json

    std::string EngineConfig;
    {
      std::ifstream Fs{std::filesystem::u8path(std::string_view{
                           reinterpret_cast<const char *>(Builders[1].data()),
                           Builders[1].size()}),
                       std::ios_base::binary};
      Fs.seekg(0, std::ios::end);
      const size_t Size = static_cast<size_t>(Fs.tellg());
      Fs.seekg(0, std::ios::beg);
      EngineConfig.resize(Size);
      Fs.read(EngineConfig.data(), Size);
    }

    if constexpr (tensorrt_llm::executor::kTensorRtLlmVersion == "0.13.0"sv ||
                  tensorrt_llm::executor::kTensorRtLlmVersion == "0.14.0"sv) {
      GraphRef.Executor = std::make_unique<tensorrt_llm::executor::Executor>(
          tensorrt_llm::executor::BufferView{Engine.data(), Engine.size()},
          EngineConfig, tensorrt_llm::executor::ModelType::kDECODER_ONLY,
          GraphRef.ExecutorConfig);
    }
    if constexpr (tensorrt_llm::executor::kTensorRtLlmVersion == "0.12.0"sv) {
      GraphRef.Executor = std::make_unique<tensorrt_llm::executor::Executor>(
          Engine, EngineConfig,
          tensorrt_llm::executor::ModelType::kDECODER_ONLY,
          GraphRef.ExecutorConfig);
    }

    return ErrNo::Success;
  } catch (std::bad_alloc &) {
    return ErrNo::InvalidArgument;
  }
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  try {
    Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);

    ContextId = static_cast<uint32_t>(Env.NNContext.size() - 1);

    return ErrNo::Success;
  } catch (std::bad_alloc &) {
    return ErrNo::InvalidArgument;
  }
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t Index,
                               const TensorData &Tensor) noexcept {
  try {
    auto &CxtRef = Env.NNContext[ContextId].get<Context>();
    auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

    if (Index == 1) {
      std::string_view Metadata(
          reinterpret_cast<const char *>(Tensor.Tensor.data()),
          Tensor.Tensor.size());
      simdjson::dom::parser Parser;
      simdjson::dom::object Doc;
      if (Parser.parse(Metadata).get(Doc)) {
        spdlog::error("[WASI-NN] TensorRT backend: Parse metadata error."sv);
        Env.NNGraph.pop_back();
        return ErrNo::InvalidEncoding;
      }
      return ErrNo::Success;
    }

    std::vector<tensorrt_llm::executor::TokenIdType> InputTokens =
        GraphRef.Tok.parse(Tensor.Tensor);
    tensorrt_llm::executor::Request Request{
        std::move(InputTokens),
        /*maxNewTokens*/ GraphRef.MaxNewTokens,
        /*streaming*/ false,
        tensorrt_llm::executor::SamplingConfig{
            /*beamWidth*/ 1,
            /*topK*/ std::nullopt,
            /*topP*/ std::nullopt,
            /*topPMin*/ std::nullopt,
            /*topPResetIds*/ std::nullopt,
            /*topPDecay*/ std::nullopt,
            /*randomSeed*/ std::nullopt,
            /*temperature*/ std::nullopt,
            /*minLength*/ std::nullopt,
            /*beamSearchDiversityRate*/ std::nullopt,
            /*repetitionPenalty*/ std::nullopt,
            /*presencePenalty*/ std::nullopt,
            /*frequencyPenalty*/ std::nullopt,
            /*lengthPenalty*/ std::nullopt,
            /*earlyStopping*/ std::nullopt,
            /*noRepeatNgramSize*/ std::nullopt,
        },
        tensorrt_llm::executor::OutputConfig{/*returnLogProbs*/ false,
                                             /*returnContextLogits*/ false,
                                             /*returnGenerationLogits*/ false,
                                             /*excludeInputFromOutput*/ true,
                                             /*returnEncoderOutput*/ false},
        /*endId*/ GraphRef.Tok.EOTId};
    CxtRef.RequestId.emplace(
        GraphRef.Executor->enqueueRequest(std::move(Request)));

    return ErrNo::Success;
  } catch (std::bad_alloc &) {
    return ErrNo::InvalidArgument;
  }
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  try {
    auto &CxtRef = Env.NNContext[ContextId].get<Context>();
    if (!CxtRef.RequestId) {
      return ErrNo::InvalidEncoding;
    }

    auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
    for (auto &Response :
         GraphRef.Executor->awaitResponses(*CxtRef.RequestId)) {
      if (Response.hasError()) {
        spdlog::error("[WASI-NN] TensorRT backend error: {}"sv,
                      Response.getErrorMsg());
        return ErrNo::InvalidEncoding;
      } else {
        auto Result = Response.getResult();
        for (auto &TokenIds : Result.outputTokenIds) {
          auto String = GraphRef.Tok.unparse(TokenIds);
          const auto Size = std::min(String.size(), OutBuffer.size());
          std::copy_n(String.begin(), Size, OutBuffer.begin());
          BytesWritten = Size;
        }
      }
    }
    return ErrNo::Success;
  } catch (std::bad_alloc &) {
    return ErrNo::InvalidArgument;
  }
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return ErrNo::Success;
}
} // namespace WasmEdge::Host::WASINN::TensorRT
#endif
