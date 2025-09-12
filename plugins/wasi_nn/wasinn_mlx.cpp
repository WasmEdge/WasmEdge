// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_mlx.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

#include "MLX/mlx/base.h"
#include "MLX/model/converter.h"
#include "MLX/model/gemma3/gemma3.h"
#include "MLX/model/llm/registry.h"
#include "MLX/model/llm/transformer.h"
#include "MLX/model/utils.h"
#include "MLX/model/whisper/whisper.h"
#include "MLX/model/whisper_transcribe.h"
#include "MLX/prompt/prompt.h"
#include <memory>
#include <mlx/array.h>

#include <simdjson.h>
#endif

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

mx::array fromBytes(const Span<uint8_t> &Bytes) {
  if (Bytes.size() < 9) {
    spdlog::error(
        "[WASI-NN] MLX backend: Tensor data must be at least 9 bytes long, current size: {}."sv,
        Bytes.size());
    return mx::array({0.0f});
  }

  size_t Offset = 0;

  uint8_t RtypeValue = Bytes[Offset];
  Offset += 1;

  uint32_t DimBufLen;
  std::memcpy(&DimBufLen, &Bytes[Offset], 4);
  Offset += 4;

  std::vector<int> Shape;
  for (size_t I = 0; I < DimBufLen; I += 4) {
    uint32_t Dim;
    std::memcpy(&Dim, &Bytes[Offset + I], 4);
    Shape.push_back(static_cast<int>(Dim));
  }
  Offset += DimBufLen;

  uint32_t DataBufLen;
  std::memcpy(&DataBufLen, &Bytes[Offset], 4);
  Offset += 4;

  const void *DataPtr = &Bytes[Offset];
  switch (RtypeValue) {
  case 0: { // F16
    return mx::array(static_cast<const uint16_t *>(DataPtr), Shape,
                     mx::float16);
  }
  case 1: { // F32
    return mx::array(static_cast<const float *>(DataPtr), Shape, mx::float32);
  }
  case 2: { // F64
    return mx::array(static_cast<const double *>(DataPtr), Shape, mx::float64);
  }
  case 3: { // U8
    return mx::array(static_cast<const uint8_t *>(DataPtr), Shape, mx::uint8);
  }
  case 4: { // I32
    return mx::array(static_cast<const int32_t *>(DataPtr), Shape, mx::int32);
  }
  case 5: { // I64
    return mx::array(static_cast<const int64_t *>(DataPtr), Shape, mx::int64);
  }
  default:
    spdlog::error("[WASI-NN] MLX backend: Unsupported rtype: {}", RtypeValue);
    return mx::array({0.0f});
  }
}

std::vector<uint8_t> toBytes(const mx::array &Arr) {
  std::vector<uint8_t> Result;

  uint8_t RtypeValue;
  switch (Arr.dtype()) {
  case mx::float16:
    RtypeValue = 0;
    break;
  case mx::float32:
    RtypeValue = 1;
    break;
  case mx::float64:
    RtypeValue = 2;
    break;
  case mx::uint8:
    RtypeValue = 3;
    break;
  case mx::int32:
    RtypeValue = 4;
    break;
  case mx::int64:
    RtypeValue = 5;
    break;
  default:
    spdlog::error(
        "[WASI-NN] MLX backend: Unsupported dtype to convert to Processor Tensor"sv);
    return Result;
  }
  Result.push_back(RtypeValue);

  std::vector<uint8_t> DimBuf;
  auto Shape = Arr.shape();
  for (int Dim : Shape) {
    uint32_t DimData = static_cast<uint32_t>(Dim);
    const uint8_t *DimBytes = reinterpret_cast<const uint8_t *>(&DimData);
    DimBuf.insert(DimBuf.end(), DimBytes, DimBytes + 4);
  }

  uint32_t DimBufLen = static_cast<uint32_t>(DimBuf.size());
  const uint8_t *DimLenBytes = reinterpret_cast<const uint8_t *>(&DimBufLen);
  Result.insert(Result.end(), DimLenBytes, DimLenBytes + 4);

  Result.insert(Result.end(), DimBuf.begin(), DimBuf.end());

  std::vector<uint8_t> DataBuf;
  mx::eval(Arr);

  switch (Arr.dtype()) {
  case mx::float16: {
    auto *Data = Arr.data<uint16_t>();
    size_t ByteSize = Arr.nbytes();
    const uint8_t *DataBytes = reinterpret_cast<const uint8_t *>(Data);
    DataBuf.insert(DataBuf.end(), DataBytes, DataBytes + ByteSize);
    break;
  }
  case mx::float32: {
    auto *Data = Arr.data<float>();
    size_t ByteSize = Arr.nbytes();
    const uint8_t *DataBytes = reinterpret_cast<const uint8_t *>(Data);
    DataBuf.insert(DataBuf.end(), DataBytes, DataBytes + ByteSize);
    break;
  }
  case mx::float64: {
    auto *Data = Arr.data<double>();
    size_t ByteSize = Arr.nbytes();
    const uint8_t *DataBytes = reinterpret_cast<const uint8_t *>(Data);
    DataBuf.insert(DataBuf.end(), DataBytes, DataBytes + ByteSize);
    break;
  }
  case mx::uint8: {
    auto *Data = Arr.data<uint8_t>();
    size_t ByteSize = Arr.nbytes();
    DataBuf.insert(DataBuf.end(), Data, Data + ByteSize);
    break;
  }
  case mx::int32: {
    auto *Data = Arr.data<int32_t>();
    size_t ByteSize = Arr.nbytes();
    const uint8_t *DataBytes = reinterpret_cast<const uint8_t *>(Data);
    DataBuf.insert(DataBuf.end(), DataBytes, DataBytes + ByteSize);
    break;
  }
  case mx::int64: {
    auto *Data = Arr.data<int64_t>();
    size_t ByteSize = Arr.nbytes();
    const uint8_t *DataBytes = reinterpret_cast<const uint8_t *>(Data);
    DataBuf.insert(DataBuf.end(), DataBytes, DataBytes + ByteSize);
    break;
  }
  default:
    spdlog::error("[WASI-NN] MLX backend: Unsupported MLX dtype for conversion "
                  "to Rust Tensor"sv);
    break;
  }

  uint32_t DataBufLen = static_cast<uint32_t>(DataBuf.size());
  const uint8_t *DataLenBytes = reinterpret_cast<const uint8_t *>(&DataBufLen);
  Result.insert(Result.end(), DataLenBytes, DataLenBytes + 4);
  Result.insert(Result.end(), DataBuf.begin(), DataBuf.end());

  return Result;
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
        "[WASI-NN] MLX backend: Lack model weight or required metadata (model_type)."sv);
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
  if (Doc.at_key("quantization").error() == simdjson::SUCCESS) {
    auto QuantResult = Doc["quantization"].get_object();
    auto Err = QuantResult.value()["group_size"].get<uint64_t>().get(
        GraphRef.GroupSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the group size from quantization option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    Err = QuantResult.value()["bits"].get<uint64_t>().get(GraphRef.QBits);
    if (Err) {
      spdlog::error(
          "[WASI-NN] MLX backend: Unable to retrieve the group size from quantization option."sv);
      Env.deleteGraph(GId);
      return ErrNo::InvalidArgument;
    }
    GraphRef.IsQuantized = true;
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
    } else if (GraphRef.ModelType == "gemma3") {
      auto Weight = mx::load_safetensors(ModelFilePath);
      Weights.insert(Weight.first.begin(), Weight.first.end());
    } else if (GraphRef.ModelType == "whisper") {
      auto Weight = mx::load_safetensors(ModelFilePath);
      Weights.insert(Weight.first.begin(), Weight.first.end());
    } else {
      spdlog::error("[WASI-NN] MLX backend: Model type {} not supported."sv,
                    GraphRef.ModelType);
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
  } else if (GraphRef.ModelType == "gemma3") {
    auto Obj = Doc.get_object();
    gemma3::ModelConfig ModelConfigObj =
        gemma3::ModelConfig::fromDict(Obj.value());
    ModelConfigObj.VisionConfig =
        (Obj.at_key("vision_config").error() == simdjson::SUCCESS)
            ? gemma3::VisionConfig::fromDict(
                  Obj["vision_config"].get_object().value())
            : gemma3::VisionConfig();
    ModelConfigObj.TextConfig =
        (Obj.at_key("text_config").error() == simdjson::SUCCESS)
            ? gemma3::TextConfig::fromDict(
                  Obj["text_config"].get_object().value())
            : gemma3::TextConfig();
    GraphRef.Model = std::dynamic_pointer_cast<nn::Module>(
        std::make_shared<gemma3::Model>(gemma3::Model(ModelConfigObj)));
    Weights = std::dynamic_pointer_cast<gemma3::Model>(GraphRef.Model)
                  ->sanitize(Weights);
    Weights =
        gemma3::VisionModel(ModelConfigObj.VisionConfig).sanitize(Weights);
    GraphRef.ModelArch = "vlm";
  } else if (GraphRef.ModelType == "whisper") {
    auto Obj = Doc.get_object();
    whisper::ModelDimensions DefaultDims =
        whisper::ModelDimensions::fromDict(Obj.value());
    GraphRef.Model = std::dynamic_pointer_cast<nn::Module>(
        std::make_shared<whisper::Whisper>(DefaultDims));
    GraphRef.ModelArch = "whisper";
  } else {
    spdlog::error("[WASI-NN] MLX backend: Model type {} not supported."sv,
                  GraphRef.ModelType);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
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
  } else if (GraphRef.ModelArch == "llm") {
    spdlog::error("[WASI-NN] MLX backend: Tokenizer path not found."sv);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }

  if (GraphRef.QBits != 0 && GraphRef.GroupSize != 0 && GraphRef.IsQuantized) {
    spdlog::info(
        "[WASI-NN] MLX backend: load Quantized model with q_bits: {} and group_size: {}"sv,
        GraphRef.QBits, GraphRef.GroupSize);
    GraphRef.Model->toQuantized(GraphRef.GroupSize, GraphRef.QBits, "",
                                Weights);
  }

  // Load weight.
  if (GraphRef.ModelType == "tiny_llama_1.1B_chat_v1.0") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "llama_3_8b") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "llama_2_7b_chat_hf") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "gemma3") {
    GraphRef.Model->update(Weights);
  } else if (GraphRef.ModelType == "whisper") {
    GraphRef.Model->update(Weights);
  } else {
    spdlog::error("[WASI-NN] MLX backend: Model type {} not supported."sv,
                  GraphRef.ModelType);
    Env.deleteGraph(GId);
    return ErrNo::InvalidArgument;
  }

  if (GraphRef.QBits != 0 && GraphRef.GroupSize != 0 && !GraphRef.IsQuantized) {
    spdlog::info(
        "[WASI-NN] MLX backend: Quantize model with q_bits: {} and group_size: {}"sv,
        GraphRef.QBits, GraphRef.GroupSize);
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
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (GraphRef.ModelArch == "llm") {
    CxtRef.Inputs = LLMInput();
  } else if (GraphRef.ModelArch == "vlm") {
    CxtRef.Inputs = VLMInput();
  } else if (GraphRef.ModelArch == "whisper") {
    CxtRef.Inputs = WhisperInput();
  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Model architecture {} not supported."sv,
        GraphRef.ModelArch);
    Env.deleteContext(ContextId);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: setInput"sv);
  }

  if (GraphRef.ModelArch == "llm") {
    std::get<LLMInput>(CxtRef.Inputs).Prompt =
        std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                    Tensor.Tensor.size());
  } else if (GraphRef.ModelArch == "vlm") {
    if (Index == 0) {
      std::get<VLMInput>(CxtRef.Inputs).Prompt = fromBytes(Tensor.Tensor);
    } else if (Index == 1) {
      std::get<VLMInput>(CxtRef.Inputs).Pixel = fromBytes(Tensor.Tensor);
    } else if (Index == 2) {
      std::get<VLMInput>(CxtRef.Inputs).Mask = fromBytes(Tensor.Tensor);
    } else {
      spdlog::error("[WASI-NN] MLX backend: Index out of range."sv);
      return ErrNo::InvalidArgument;
    }
  } else if (GraphRef.ModelArch == "whisper") {
    std::get<WhisperInput>(CxtRef.Inputs).Audio =
        std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                    Tensor.Tensor.size());
  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Model architecture {} not supported."sv,
        GraphRef.ModelArch);
    return ErrNo::InvalidArgument;
  }
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
  if (GraphRef.ModelArch == "llm") {
    auto *Output = std::get_if<LLMOutput>(&CxtRef.Outputs);
    if (Output != nullptr) {
      std::copy_n(Output->Answer.data(), Output->Answer.length(),
                  OutBuffer.data());
      BytesWritten = Output->Answer.length();
    } else {
      spdlog::error("[WASI-NN] MLX backend: No output found."sv);
      return ErrNo::InvalidArgument;
    }
  } else if (GraphRef.ModelArch == "vlm") {
    auto *Output = std::get_if<VLMOutput>(&CxtRef.Outputs);
    if (Output != nullptr) {
      auto OutputBytes = toBytes(Output->Answer);
      std::copy_n(OutputBytes.data(), OutputBytes.size(), OutBuffer.data());
      BytesWritten = OutputBytes.size();
    } else {
      spdlog::error("[WASI-NN] MLX backend: No output found."sv);
      return ErrNo::InvalidArgument;
    }
  } else if (GraphRef.ModelArch == "whisper") {
    auto *Output = std::get_if<whisper::TranscribeResult>(&CxtRef.Outputs);
    if (Output != nullptr) {
      std::string Text = Output->Text;
      std::copy_n(Text.data(), Text.length(), OutBuffer.data());
      BytesWritten = Text.length();
    } else {
      spdlog::error("[WASI-NN] MLX backend: No output found."sv);
      return ErrNo::InvalidArgument;
    }

  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Model architecture {} not supported."sv,
        GraphRef.ModelArch);
    return ErrNo::InvalidArgument;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  const auto Start{std::chrono::steady_clock::now()};
  size_t TokenListSize = 0;

  if (GraphRef.ModelArch == "llm" && GraphRef.Tok == nullptr) {
    spdlog::error("[WASI-NN] MLX backend: Tokenizer not loaded."sv);
    return ErrNo::InvalidArgument;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: compute"sv);
  }
  if (GraphRef.ModelArch == "llm") {
    auto Result =
        std::dynamic_pointer_cast<llm::Transformer>(GraphRef.Model)
            ->generate(std::get<LLMInput>(CxtRef.Inputs).Prompt,
                       GraphRef.Prmopt, GraphRef.MaxToken, false, GraphRef.Tok);
    CxtRef.Outputs = LLMOutput({Result.Answer});
    TokenListSize = Result.TokenList.size();
  } else if (GraphRef.ModelArch == "vlm") {
    auto &Input = std::get<VLMInput>(CxtRef.Inputs);
    std::map<std::string, std::variant<mx::array, int, float, std::string>>
        Kwargs;
    Kwargs.insert({"input_ids", Input.Prompt});
    Kwargs.insert({"pixel_values", Input.Pixel});
    Kwargs.insert({"mask", Input.Mask});
    auto TokenList = std::dynamic_pointer_cast<gemma3::Model>(GraphRef.Model)
                         ->generate({}, std::nullopt, false, Kwargs);
    auto TokenArray =
        mx::array(TokenList.data(), {static_cast<int>(TokenList.size())});
    CxtRef.Outputs = VLMOutput({TokenArray});
    TokenListSize = TokenList.size();
  } else if (GraphRef.ModelArch == "whisper") {
    CxtRef.Outputs = whisper::transcribe(
        std::get<WhisperInput>(CxtRef.Inputs).Audio,
        std::dynamic_pointer_cast<whisper::Whisper>(GraphRef.Model), false);
  } else {
    spdlog::error(
        "[WASI-NN] MLX backend: Model architecture {} not supported."sv,
        GraphRef.ModelArch);
    return ErrNo::InvalidArgument;
  }
  const auto End{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> ElapsedSeconds{End - Start};
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] MLX backend: Generate {} tokens."sv, TokenListSize);
    spdlog::info("Elapsed time: {} s. TPS: {}.", ElapsedSeconds.count(),
                 TokenListSize / ElapsedSeconds.count());
  }
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
