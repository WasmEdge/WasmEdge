// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_mlx.h"
#include "wasinnenv.h"
#include <memory>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

#include "MLX/mlx/base.h"
#include "MLX/model/converter.h"
#include "MLX/model/gemma3/gemma3.h"
#include "MLX/model/llm/registry.h"
#include "MLX/model/llm/transformer.h"
#include "MLX/model/utils.h"
#include "MLX/prompt/prompt.h"
#include <mlx/array.h>

#include <simdjson.h>
#endif

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
std::string loadBytesFromFile(const std::string &Path) {
  std::ifstream Fs(Path, std::ios::in | std::ios::binary);
  if (Fs.fail()) {
    spdlog::error("[WASI-NN] MLX backend: Cannot open {}."sv, Path);
    return "";
  }
  std::string Data;
  Fs.seekg(0, std::ios::end);
  const size_t Size = static_cast<size_t>(Fs.tellg());
  Fs.seekg(0, std::ios::beg);
  Data.resize(Size);
  Fs.read(Data.data(), Size);
  return Data;
}

enum AnswerSataus {
  STOP,
  WAIT,
  GO,
};

AnswerSataus answerSataus(std::string Text, std::string End) {
  if (endsWith(Text, End)) {
    return STOP;
  }
  for (int Idx = 1; Idx < static_cast<int>(End.size()); Idx++) {
    if (endsWith(Text, End.substr(0, Idx))) {
      return WAIT;
    }
  }
  return GO;
}

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders, WASINN::Device,
                           uint32_t &GraphId) noexcept {
  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::MLX);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: Load."sv);
  }
  std::string TokenizerPath;
  // Parse metadata.
  if (Builders.size() <= 1) {
    spdlog::error(
        "[WASI-NN] MLX backend: Lack model weight or required metadata (tokenizer, model_type)."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }
  const std::string Metadata = std::string(
      reinterpret_cast<char *>(Builders.back().data()), Builders.back().size());
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    spdlog::error("[WASI-NN] MLX backend: Parse metadata error"sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidEncoding;
  }
  if (Doc.at_key("model_type").error() == simdjson::SUCCESS) {
    std::string_view ModelType;
    auto Err = Doc["model_type"].get<std::string_view>().get(ModelType);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the model_type option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.ModelType = ModelType;
  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Unable to retrieve the model_type option."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }
  if (Doc.at_key("enable_debug_log").error() == simdjson::SUCCESS) {
    bool EnableDebugLog;
    auto Err = Doc["enable_debug_log"].get<bool>().get(EnableDebugLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the enable_debug_log option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.EnableDebugLog = EnableDebugLog;
  }
  if (Doc.at_key("tokenizer").error() == simdjson::SUCCESS) {
    std::string_view TokenizerPathView;
    auto Err = Doc["tokenizer"].get<std::string_view>().get(TokenizerPathView);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the tokenizer option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    TokenizerPath = TokenizerPathView;
  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Unable to retrieve the tokenizer option."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }
  if (Doc.at_key("max_token").error() == simdjson::SUCCESS) {
    uint64_t MaxToken;
    auto Err = Doc["max_token"].get<uint64_t>().get(MaxToken);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the max_token option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.MaxToken = MaxToken;
  }
  if (Doc.at_key("q_bits").error() == simdjson::SUCCESS &&
      Doc.at_key("group_size").error() == simdjson::SUCCESS &&
      Doc.at_key("is_quantized").error() == simdjson::SUCCESS) {
    uint64_t QBits;
    uint64_t GroupSize;
    bool IsQuantized;
    auto ErrQBits = Doc["q_bits"].get<uint64_t>().get(QBits);
    auto ErrGroupSize = Doc["group_size"].get<uint64_t>().get(GroupSize);
    auto ErrIsQuantized = Doc["is_quantized"].get<bool>().get(IsQuantized);
    if (ErrQBits || ErrGroupSize || ErrIsQuantized) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the q_bits or group_size option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.IsQuantized = IsQuantized;
    GraphRef.QBits = QBits;
    GraphRef.GroupSize = GroupSize;
  }

  // Load tokenizer.
  if (!TokenizerPath.empty()) {
    auto Bytes = loadBytesFromFile(TokenizerPath);
    if (Bytes.empty()) {
      spdlog::error("[WASI-NN] MLX backend: Load tokenizer failed."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Tok = tokenizers::Tokenizer::FromBlobJSON(Bytes);
  } else {
    spdlog::error("[WASI-NN] MLX backend: Tokenizer path not found."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }

  std::unordered_map<std::string, mx::array> Weights;
  // Handle the model path.
  for (size_t Idx = 0; Idx < Builders.size() - 1; Idx++) {
    auto WeightData = Builders[Idx];
    const std::string BinModel(reinterpret_cast<char *>(WeightData.data()),
                               WeightData.size());
    spdlog::info("[WASI-NN] MLX BinModel: {}"sv, BinModel.size());
    if (BinModel.size() == 0) {
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    std::string ModelFilePath;
    if (BinModel.substr(0, 8) == "preload:"sv) {
      ModelFilePath = BinModel.substr(8);
    } else {
      if (GraphRef.EnableDebugLog) {
        spdlog::info(
            "[WASI-NN][Debug] MLX backend: Model path not found in nn-preload, "
            "write model into a tmpfile."sv);
      }
      // Write model to file.
      // TODO: handle different model format.
      ModelFilePath = "MLX" + std::to_string(Idx) + ".safetensors";
      std::ofstream TempFile(ModelFilePath, std::ios::out | std::ios::binary);
      if (!TempFile) {
        spdlog::error(
            "[WASI-NN] MLX backend: Failed to create the temporary file. "sv);
        Env.deleteGraph(GId);
        return ErrNo::InvalidArgument;
      }
      TempFile.write(BinModel.data(), BinModel.size());
      TempFile.close();
      if (GraphRef.EnableDebugLog) {
        spdlog::info(
            "[WASI-NN][Debug] MLX backend: Write model into a tmpfile...Done"sv);
      }
    }

    if (GraphRef.ModelType == "tiny_llama_1.1B_chat_v1.0") {
      auto Weight = llamaToMlxllm(ModelFilePath);
      Weights.insert(Weight.begin(), Weight.end());
    } else if (GraphRef.ModelType == "llama_3_8b") {
      auto Weight = llamaToMlxllm(ModelFilePath);
      Weights.insert(Weight.begin(), Weight.end());
    } else if (GraphRef.ModelType == "llama_2_7b_chat_hf") {
      auto Weight = llamaToMlxllm(ModelFilePath);
      Weights.insert(Weight.begin(), Weight.end());
    } else {
      spdlog::error("[WASI-NN] MLX backend: Model type not supported."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
  }

  // Create Model.
  if (GraphRef.ModelType == "tiny_llama_1.1B_chat_v1.0") {
    GraphRef.Model = llm::tinyLlama11BChatV10();
    GraphRef.Prmopt = TinyLLaMAPrompt();
    GraphRef.ModelArch = "llm";
  } else if (GraphRef.ModelType == "llama_3_8b") {
    GraphRef.Model = llm::llama38b();
    GraphRef.Prmopt = LLaMA3Prompt();
    GraphRef.ModelArch = "llm";
  } else if (GraphRef.ModelType == "llama_2_7b_chat_hf") {
    GraphRef.Model = llm::llama27bChat();
    GraphRef.Prmopt = LLaMA2Prompt();
    GraphRef.ModelArch = "llm";
  } else if (GraphRef.ModelType == "gemma_3") {
    auto Obj = Doc.get_object();
    gemma3::ModelConfig ModelConfigObj =
        gemma3::ModelConfig::fromDict(Obj.value());
    ModelConfigObj.VisionConfig = gemma3::VisionConfig::fromDict(
        Obj["vision_config"].get_object().value());
    ModelConfigObj.TextConfig =
        gemma3::TextConfig::fromDict(Obj["text_config"].get_object().value());
    GraphRef.Model = std::dynamic_pointer_cast<nn::Module>(
        std::make_shared<gemma3::Model>(gemma3::Model(ModelConfigObj)));
    GraphRef.ModelArch = "vlm";
  } else {
    spdlog::error("[WASI-NN] MLX backend: Model type not supported."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }

  if (GraphRef.QBits != 0 && GraphRef.GroupSize != 0 && GraphRef.IsQuantized) {
    GraphRef.Model->toQuantized(GraphRef.GroupSize, GraphRef.QBits);
  }

  // Load weight.
  if (GraphRef.ModelType == "tiny_llama_1.1B_chat_v1.0") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "llama_3_8b") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "llama_2_7b_chat_hf") {
    GraphRef.Model->update(Weights);
  } else {
    spdlog::error("[WASI-NN] MLX backend: Model type not supported."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }

  if (GraphRef.QBits != 0 && GraphRef.GroupSize != 0 && !GraphRef.IsQuantized) {
    GraphRef.Model->toQuantized(GraphRef.GroupSize, GraphRef.QBits);
  }

  GraphId = GId;
  Env.NNGraph[GId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  Env.NNContext[ContextId].setReady();
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: setInput"sv);
  }
  CxtRef.Inputs =
      std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                  Tensor.Tensor.size());
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                                uint32_t, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: getOutput"sv);
  }
  std::string StringTmp(reinterpret_cast<const char *>(CxtRef.Outputs.data()),
                        CxtRef.Outputs.size());
  std::copy_n(StringTmp.data(), StringTmp.length(), OutBuffer.data());
  BytesWritten = StringTmp.length();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.Tok == nullptr) {
    spdlog::error("[WASI-NN] MLX backend: Tokenizer not loaded."sv);
    return ErrNo::InvalidArgument;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: compute"sv);
  }
  CxtRef.Outputs = std::dynamic_pointer_cast<llm::Transformer>(GraphRef.Model)
                       ->generate(CxtRef.Inputs, GraphRef.Prmopt,
                                  GraphRef.MaxToken, false, GraphRef.Tok)
                       .Answer;
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
