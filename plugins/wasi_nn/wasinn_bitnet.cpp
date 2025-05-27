// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_bitnet.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include <ggml-bitnet.h>
#include <ggml.h>
#include <thread>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <random>
#include <regex>
#endif

namespace WasmEdge::Host::WASINN::BitNet {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET

namespace {

// Simple tokenizer for BitNet models (based on character-level tokenization)
std::vector<int32_t> bitnet_tokenize(const std::string &text, int32_t n_vocab) {
  std::vector<int32_t> tokens;
  
  // Simple character-level tokenization
  for (char c : text) {
    int32_t token = static_cast<int32_t>(static_cast<uint8_t>(c));
    if (token < n_vocab) {
      tokens.push_back(token);
    }
  }
  
  // Add BOS token if vocabulary supports it
  if (n_vocab > 256) {
    tokens.insert(tokens.begin(), 1); // BOS token
  }
  
  return tokens;
}

// Convert tokens back to text
std::string bitnet_detokenize(const std::vector<int32_t> &tokens, int32_t n_vocab) {
  std::string text;
  
  for (int32_t token : tokens) {
    // Skip special tokens
    if (token == 0 || token == 1 || token == 2) continue;
    
    // Convert to character if in valid range
    if (token >= 32 && token <= 126) {
      text += static_cast<char>(token);
    } else if (token >= 0 && token <= 255) {
      text += static_cast<char>(token);
    }
  }
  
  return text;
}

// Load BitNet model using GGML-BitNet interface
bool load_bitnet_model(const std::string &model_path, Graph &graph) {
  try {
    // Initialize GGML-BitNet
    ggml_bitnet_init();
    
    // Read model file
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      spdlog::error("[WASI-NN] BitNet: Cannot open model file: {}", model_path);
      return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    graph.ModelData.resize(size);
    if (!file.read(reinterpret_cast<char*>(graph.ModelData.data()), size)) {
      spdlog::error("[WASI-NN] BitNet: Failed to read model file");
      return false;
    }
    
    // Parse model header (simplified) -> for poc 
    // In a the main implementation, this would parse the BitNet model format
    const uint8_t *data = graph.ModelData.data();
    size_t offset = 0;
    
    // Read model hyperparameters (example format)
    if (size >= 32) {
      std::memcpy(&graph.n_vocab, data + offset, sizeof(int32_t)); offset += 4;
      std::memcpy(&graph.n_ctx, data + offset, sizeof(int32_t)); offset += 4;
      std::memcpy(&graph.n_embd, data + offset, sizeof(int32_t)); offset += 4;
      std::memcpy(&graph.n_head, data + offset, sizeof(int32_t)); offset += 4;
      std::memcpy(&graph.n_layer, data + offset, sizeof(int32_t)); offset += 4;
      std::memcpy(&graph.n_ff, data + offset, sizeof(int32_t)); offset += 4;
      
      spdlog::info("[WASI-NN] BitNet model loaded: vocab={}, ctx={}, embd={}, heads={}, layers={}",
                   graph.n_vocab, graph.n_ctx, graph.n_embd, graph.n_head, graph.n_layer);
    } else {
      // Default values for testing
      graph.n_vocab = 32000;
      graph.n_ctx = 512;
      graph.n_embd = 4096;
      graph.n_head = 32;
      graph.n_layer = 32;
      graph.n_ff = 11008;
    }
    
    // Initialize GGML context for BitNet operations
    struct ggml_init_params params = {
      .mem_size = 1024 * 1024 * 1024, // 1GB
      .mem_buffer = nullptr,
      .no_alloc = false,
    };
    
    graph.GgmlContext = ggml_init(params);
    if (!graph.GgmlContext) {
      spdlog::error("[WASI-NN] BitNet: Failed to initialize GGML context");
      return false;
    }
    
    graph.ModelPath = model_path;
    return true;
    
  } catch (const std::exception &e) {
    spdlog::error("[WASI-NN] BitNet model loading failed: {}", e.what());
    return false;
  }
}

// Perform BitNet inference using quantized operations
bool bitnet_inference(Graph &graph, Context &context, 
                     const std::vector<int32_t> &input_tokens,
                     std::vector<int32_t> &output_tokens) {
  
  if (!graph.GgmlContext) {
    spdlog::error("[WASI-NN] BitNet: Invalid GGML context");
    return false;
  }
  
  try {
    // Set up BitNet parameters
    ggml_bitnet_set_n_threads(context.n_threads);
    
    const int n_embd = graph.n_embd;
    const int n_vocab = graph.n_vocab;
    const int n_ctx = std::min(static_cast<int>(input_tokens.size() + context.n_predict), graph.n_ctx);
    
    // Create input tensor
    struct ggml_tensor *input_tensor = ggml_new_tensor_1d(graph.GgmlContext, GGML_TYPE_I32, input_tokens.size());
    if (!input_tensor) {
      spdlog::error("[WASI-NN] BitNet: Failed to create input tensor");
      return false;
    }
    
    // Copy input tokens
    std::memcpy(input_tensor->data, input_tokens.data(), input_tokens.size() * sizeof(int32_t));
    
    // Create embedding tensor for BitNet quantized operations
    struct ggml_tensor *embd_tensor = ggml_new_tensor_2d(graph.GgmlContext, GGML_TYPE_F32, n_embd, input_tokens.size());
    if (!embd_tensor) {
      spdlog::error("[WASI-NN] BitNet: Failed to create embedding tensor");
      return false;
    }
    
    // Simulate BitNet quantized embedding lookup
    // In real implementation, this would use actual BitNet quantized weights
    float *embd_data = reinterpret_cast<float*>(embd_tensor->data);
    std::random_device rd;
    std::mt19937 gen(context.seed >= 0 ? context.seed : rd());
    std::normal_distribution<float> dist(0.0f, 0.1f);
    
    for (size_t i = 0; i < input_tokens.size() * n_embd; ++i) {
      embd_data[i] = dist(gen);
    }
    
    // Simulate BitNet transformer forward pass with quantized operations
    for (int layer = 0; layer < graph.n_layer; ++layer) {
      // Create attention weights (would be quantized in real BitNet)
      struct ggml_tensor *attn_weights = ggml_new_tensor_2d(graph.GgmlContext, GGML_TYPE_Q4_0, n_embd, n_embd);
      if (!attn_weights) {
        spdlog::warn("[WASI-NN] BitNet: Failed to create attention weights for layer {}", layer);
        continue;
      }
      
      // Check if BitNet multiplication is supported
      if (ggml_bitnet_can_mul_mat(attn_weights, embd_tensor, embd_tensor)) {
        // Get workspace size for BitNet operations
        size_t wsize = ggml_bitnet_mul_mat_get_wsize(attn_weights, embd_tensor, embd_tensor);
        
        // Initialize BitNet multiplication task
        void *workspace = malloc(wsize);
        if (workspace) {
          int bits = ggml_bitnet_get_type_bits(attn_weights->type);
          ggml_bitnet_mul_mat_task_init(embd_tensor->data, workspace, nullptr, nullptr, 
                                       n_embd, n_embd, input_tokens.size(), bits);
          
          // Compute BitNet quantized matrix multiplication
          ggml_bitnet_mul_mat_task_compute(attn_weights->data, nullptr, workspace, nullptr, nullptr,
                                         embd_tensor->data, n_embd, n_embd, input_tokens.size(), bits);
          
          free(workspace);
        }
      }
    }
    
    // Generate output logits
    context.logits.resize(n_vocab);
    
    // Simulate logits computation (would use actual BitNet quantized operations in main implementation)
    for (int i = 0; i < n_vocab; ++i) {
      context.logits[i] = dist(gen);
    }
    
    // Apply temperature scaling
    if (context.temp > 0.0f) {
      for (float &logit : context.logits) {
        logit /= context.temp;
      }
    }
    
    // Apply top-k and top-p sampling
    std::vector<std::pair<float, int>> logit_pairs;
    for (int i = 0; i < n_vocab; ++i) {
      logit_pairs.emplace_back(context.logits[i], i);
    }
    
    // Sort by logit value (descending)
    std::sort(logit_pairs.begin(), logit_pairs.end(), 
              [](const auto &a, const auto &b) { return a.first > b.first; });
    
    // Apply top-k filtering
    if (context.top_k > 0 && context.top_k < n_vocab) {
      logit_pairs.resize(context.top_k);
    }
    
    // Convert logits to probabilities
    float max_logit = logit_pairs[0].first;
    float sum_exp = 0.0f;
    for (auto &pair : logit_pairs) {
      pair.first = std::exp(pair.first - max_logit);
      sum_exp += pair.first;
    }
    
    for (auto &pair : logit_pairs) {
      pair.first /= sum_exp;
    }
    
    // Apply top-p filtering
    if (context.top_p < 1.0f) {
      float cumsum = 0.0f;
      size_t last_idx = 0;
      for (size_t i = 0; i < logit_pairs.size(); ++i) {
        cumsum += logit_pairs[i].first;
        if (cumsum >= context.top_p) {
          last_idx = i + 1;
          break;
        }
      }
      logit_pairs.resize(last_idx);
    }
    
    // Sample from the filtered distribution
    std::uniform_real_distribution<float> sample_dist(0.0f, 1.0f);
    float sample = sample_dist(gen);
    
    int32_t next_token = 0;
    float cumsum = 0.0f;
    for (const auto &pair : logit_pairs) {
      cumsum += pair.first;
      if (sample <= cumsum) {
        next_token = pair.second;
        break;
      }
    }
    
    // Generate sequence
    output_tokens = input_tokens;
    
    for (int i = 0; i < context.n_predict; ++i) {
      output_tokens.push_back(next_token);
      
      // Check for end-of-sequence
      if (next_token == 0 || next_token == 2) { // EOS tokens
        break;
      }
      
      // For simplicity as an poc, generate next token based on simple heuristics
      // In main implementation, this would run the full BitNet forward pass
      next_token = (next_token + 1) % std::min(256, n_vocab);
    }
    
    return true;
    
  } catch (const std::exception &e) {
    spdlog::error("[WASI-NN] BitNet inference failed: {}", e.what());
    return false;
  }
}

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] BitNet backend expects exactly one builder, got {}",
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  if (Device != Device::CPU) {
    spdlog::error("[WASI-NN] BitNet backend currently only supports CPU target");
    return ErrNo::InvalidArgument;
  }

  // Create new graph
  GraphId = Env.newGraph(Backend::BitNet);
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();

  // Extract model data
  auto ModelData = Builders[0];
  std::string ModelPath;
  
  // Check if this is a preloaded model path
  std::string ModelStr(reinterpret_cast<const char *>(ModelData.data()), 
                       ModelData.size());
  if (ModelStr.substr(0, 8) == "preload:") {
    ModelPath = ModelStr.substr(8);
    spdlog::info("[WASI-NN] BitNet loading preloaded model from: {}", ModelPath);
  } else {
    // Write model data to temporary file
    ModelPath = "/tmp/bitnet_model.bin";
    std::ofstream OutFile(ModelPath, std::ios::binary);
    if (!OutFile) {
      spdlog::error("[WASI-NN] Failed to create temporary model file");
      return ErrNo::RuntimeError;
    }
    OutFile.write(reinterpret_cast<const char *>(ModelData.data()), 
                  ModelData.size());
    OutFile.close();
    spdlog::info("[WASI-NN] BitNet model written to temporary file: {}", ModelPath);
  }

  // Load the BitNet model
  if (!load_bitnet_model(ModelPath, GraphRef)) {
    spdlog::error("[WASI-NN] Failed to load BitNet model from: {}", ModelPath);
    return ErrNo::RuntimeError;
  }

  Env.NNGraph[GraphId].setReady();
  spdlog::info("[WASI-NN] BitNet model loaded successfully. Graph ID: {}, Vocab: {}, Context: {}", 
               GraphId, GraphRef.n_vocab, GraphRef.n_ctx);
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  if (GraphId >= Env.NNGraph.size()) {
    spdlog::error("[WASI-NN] BitNet invalid graph ID: {}", GraphId);
    return ErrNo::InvalidArgument;
  }

  auto &GraphRef = Env.NNGraph[GraphId];
  if (!GraphRef.isReady()) {
    spdlog::error("[WASI-NN] BitNet graph {} is not ready", GraphId);
    return ErrNo::InvalidArgument;
  }

  // Create execution context
  ContextId = Env.newContext(GraphId, GraphRef);
  auto &CtxRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphData = GraphRef.get<Graph>();
  
  // Initialize BitNet inference context
  struct ggml_init_params params = {
    .mem_size = 256 * 1024 * 1024, // 256MB for inference
    .mem_buffer = nullptr,
    .no_alloc = false,
  };
  
  CtxRef.InferenceContext = ggml_init(params);
  if (!CtxRef.InferenceContext) {
    spdlog::error("[WASI-NN] BitNet: Failed to initialize inference context");
    return ErrNo::RuntimeError;
  }
  
  // Set default inference parameters
  CtxRef.n_threads = std::thread::hardware_concurrency();
  CtxRef.seed = std::random_device{}();
  
  Env.NNContext[ContextId].setReady();
  spdlog::info("[WASI-NN] BitNet execution context initialized, Context ID: {}", ContextId);
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    spdlog::error("[WASI-NN] BitNet invalid context ID: {}", ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CtxRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CtxRef.GraphId].get<Graph>();
  
  if (Index == 0) {
    // Input text
    std::string InputText(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                         Tensor.Tensor.size());
    CtxRef.InputText = InputText;
    CtxRef.InputTokens = bitnet_tokenize(InputText, GraphRef.n_vocab);
    
    spdlog::debug("[WASI-NN] BitNet input text set: '{}' ({} tokens)", 
                  InputText.substr(0, 100), CtxRef.InputTokens.size());
                  
  } else if (Index == 1) {
    // Inference parameters (JSON format)
    std::string ParamsStr(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                         Tensor.Tensor.size());
    
    // Parse basic parameters (simplified JSON parsing)
    std::regex temp_regex(R"("temperature":\s*([0-9]*\.?[0-9]+))");
    std::regex top_k_regex(R"("top_k":\s*([0-9]+))");
    std::regex top_p_regex(R"("top_p":\s*([0-9]*\.?[0-9]+))");
    std::regex n_predict_regex(R"("max_tokens":\s*([0-9]+))");
    
    std::smatch match;
    if (std::regex_search(ParamsStr, match, temp_regex)) {
      CtxRef.temp = std::stof(match[1].str());
    }
    if (std::regex_search(ParamsStr, match, top_k_regex)) {
      CtxRef.top_k = std::stoi(match[1].str());
    }
    if (std::regex_search(ParamsStr, match, top_p_regex)) {
      CtxRef.top_p = std::stof(match[1].str());
    }
    if (std::regex_search(ParamsStr, match, n_predict_regex)) {
      CtxRef.n_predict = std::stoi(match[1].str());
    }
    
    spdlog::debug("[WASI-NN] BitNet parameters set: temp={}, top_k={}, top_p={}, n_predict={}",
                  CtxRef.temp, CtxRef.top_k, CtxRef.top_p, CtxRef.n_predict);
  }
  
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    spdlog::error("[WASI-NN] BitNet invalid context ID: {}", ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CtxRef = Env.NNContext[ContextId].get<Context>();
  
  if (Index == 0) {
    // Output text
    const auto &OutputText = CtxRef.OutputText;
    BytesWritten = std::min(static_cast<uint32_t>(OutputText.size()), 
                           static_cast<uint32_t>(OutBuffer.size()));
    std::copy_n(OutputText.begin(), BytesWritten, OutBuffer.begin());
    spdlog::debug("[WASI-NN] BitNet output retrieved: {} bytes", BytesWritten);
  }
  
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    spdlog::error("[WASI-NN] BitNet invalid context ID: {}", ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CtxRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CtxRef.GraphId].get<Graph>();

  if (CtxRef.InputTokens.empty()) {
    spdlog::error("[WASI-NN] BitNet no input set for computation");
    return ErrNo::InvalidArgument;
  }

  try {
    // Perform BitNet inference
    std::vector<int32_t> output_tokens;
    if (!bitnet_inference(GraphRef, CtxRef, CtxRef.InputTokens, output_tokens)) {
      spdlog::error("[WASI-NN] BitNet inference failed");
      return ErrNo::RuntimeError;
    }
    
    // Convert output tokens to text
    CtxRef.OutputTokens = output_tokens;
    CtxRef.OutputText = bitnet_detokenize(output_tokens, GraphRef.n_vocab);
    
    spdlog::info("[WASI-NN] BitNet computation completed. Output: {} tokens, {} chars",
                 output_tokens.size(), CtxRef.OutputText.size());
    return ErrNo::Success;
  } catch (const std::exception &e) {
    spdlog::error("[WASI-NN] BitNet computation failed: {}", e.what());
    return ErrNo::RuntimeError;
  }
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  return compute(Env, ContextId);
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  return getOutput(Env, ContextId, Index, OutBuffer, BytesWritten);
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    return ErrNo::InvalidArgument;
  }

  auto &CtxRef = Env.NNContext[ContextId].get<Context>();
  
  // Clear context data
  CtxRef.InputTokens.clear();
  CtxRef.OutputTokens.clear();
  CtxRef.InputText.clear();
  CtxRef.OutputText.clear();
  CtxRef.logits.clear();
  CtxRef.embeddings.clear();
  
  // Reset inference state
  CtxRef.n_past = 0;

  spdlog::info("[WASI-NN] BitNet single execution finalized");
  return ErrNo::Success;
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  if (GraphId >= Env.NNGraph.size()) {
    return ErrNo::InvalidArgument;
  }

  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  
  // Cleanup GGML context and BitNet resources
  if (GraphRef.GgmlContext) {
    ggml_free(GraphRef.GgmlContext);
    GraphRef.GgmlContext = nullptr;
  }
  
  // Clear model data
  GraphRef.ModelData.clear();
  GraphRef.ModelPath.clear();
  
  // Cleanup BitNet library
  ggml_bitnet_free();

  Env.deleteGraph(GraphId);
  spdlog::info("[WASI-NN] BitNet graph {} unloaded", GraphId);
  return ErrNo::Success;
}

#else
// Stub implementations when BitNet backend is not built
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] BitNet backend is not built. Please build with "
                "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=BitNet");
  return ErrNo::UnsupportedOperation;
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

Expect<ErrNo> computeSingle(WasiNNEnvironment &, uint32_t) noexcept {
return reportBackendNotSupported();
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &, uint32_t, uint32_t,
               Span<uint8_t>, uint32_t &) noexcept {
return reportBackendNotSupported();
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &, uint32_t) noexcept {
return reportBackendNotSupported();
}

Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
return reportBackendNotSupported();
}

#endif

} // namespace WasmEdge::Host::WASINN::BitNet