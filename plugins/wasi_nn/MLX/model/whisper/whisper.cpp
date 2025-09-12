// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "whisper.h"
#include "mlx/activations.h"
#include "mlx/base.h"
#include "mlx/convolution.h"
#include "mlx/embedding.h"
#include "mlx/linear.h"
#include "mlx/normalization.h"
#include "mlx/transformer.h"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <mlx/dtype.h>

namespace WasmEdge::Host::WASINN::MLX {

namespace whisper {

mx::array sinusoids(int Length, int Channels, float MaxTimescale) {
  assert(Channels % 2 == 0);
  float LogTimescaleIncrement = std::log(MaxTimescale) / (Channels / 2 - 1);
  mx::array InvTimescales =
      mx::exp(-LogTimescaleIncrement * mx::arange(Channels / 2));
  mx::array LengthArray = mx::arange(Length);
  mx::array LengthReshaped = reshape(LengthArray, {Length, 1});
  mx::array InvTimescalesReshaped = reshape(InvTimescales, {1, -1});
  mx::array ScaledTime = LengthReshaped * InvTimescalesReshaped;
  return mx::concatenate({mx::sin(ScaledTime), mx::cos(ScaledTime)},
                         /*axis=*/1);
}

ModelDimensions ModelDimensions::fromDict(const simdjson::dom::object &Obj) {
  ModelDimensions Dims;
  if (auto Val = Obj["n_mels"]; !Val.error()) {
    Dims.NMels = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_audio_ctx"]; !Val.error()) {
    Dims.NAudioCtx = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_audio_state"]; !Val.error()) {
    Dims.NAudioState = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_audio_head"]; !Val.error()) {
    Dims.NAudioHead = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_audio_layer"]; !Val.error()) {
    Dims.NAudioLayer = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_vocab"]; !Val.error()) {
    Dims.NVocab = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_text_ctx"]; !Val.error()) {
    Dims.NTextCtx = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_text_state"]; !Val.error()) {
    Dims.NTextState = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_text_head"]; !Val.error()) {
    Dims.NTextHead = Val.get<int64_t>();
  }
  if (auto Val = Obj["n_text_layer"]; !Val.error()) {
    Dims.NTextLayer = Val.get<int64_t>();
  }
  return Dims;
}

// MultiHeadAttention implementation
MultiHeadAttention::MultiHeadAttention(int NState, int NHead) : NHead(NHead) {
  auto Query = std::make_shared<nn::Linear>(NState, NState);
  auto Key = std::make_shared<nn::Linear>(NState, NState, /*bias=*/false);
  auto Value = std::make_shared<nn::Linear>(NState, NState);
  auto Out = std::make_shared<nn::Linear>(NState, NState);

  registerModule("query", Query);
  registerModule("key", Key);
  registerModule("value", Value);
  registerModule("out", Out);
}

std::tuple<mx::array, std::pair<mx::array, mx::array>, mx::array>
MultiHeadAttention::forward(
    const mx::array &X, const std::optional<mx::array> &Xa,
    const std::optional<mx::array> &Mask,
    const std::optional<std::pair<mx::array, mx::array>> &KvCache) {

  auto Query = std::dynamic_pointer_cast<nn::Linear>(Submodules["query"]);
  auto Key = std::dynamic_pointer_cast<nn::Linear>(Submodules["key"]);
  auto Value = std::dynamic_pointer_cast<nn::Linear>(Submodules["value"]);
  auto Out = std::dynamic_pointer_cast<nn::Linear>(Submodules["out"]);

  mx::array Q = Query->forward(X);
  mx::array K = mx::array({}), V = mx::array({});

  if (!Xa.has_value()) {
    // Self-attention
    K = Key->forward(X);
    V = Value->forward(X);
    if (KvCache.has_value()) {
      K = mx::concatenate({KvCache->first, K}, 1);
      V = mx::concatenate({KvCache->second, V}, 1);
    }
  } else if (!KvCache.has_value()) {
    // Cross-attention without cache
    K = Key->forward(*Xa);
    V = Value->forward(*Xa);
  } else {
    // Cross-attention with cache
    K = KvCache->first;
    V = KvCache->second;
  }
  auto [Wv, Qk] = qkvAttention(Q, K, V, Mask);
  return {Out->forward(Wv), {K, V}, Qk};
}

std::pair<mx::array, mx::array>
MultiHeadAttention::qkvAttention(const mx::array &Q, const mx::array &K,
                                 const mx::array &V,
                                 const std::optional<mx::array> &Mask) {

  auto Shape = Q.shape();
  int NBatch = Shape[0];
  int NCtx = Shape[1];
  int NState = Shape[2];

  double Scale = std::pow(NState / NHead, -0.25);
  Scale = std::round(Scale * 1000000) / 1000000;

  // Reshape and transpose for multi-head attention
  mx::array QReshaped = reshape(Q, {Q.shape(0), Q.shape(1), NHead, -1});
  QReshaped = transpose(QReshaped, {0, 2, 1, 3}) * Scale;

  mx::array KReshaped = reshape(K, {K.shape(0), K.shape(1), NHead, -1});
  KReshaped = transpose(KReshaped, {0, 2, 3, 1}) * Scale;

  mx::array VReshaped = reshape(V, {V.shape(0), V.shape(1), NHead, -1});
  VReshaped = transpose(VReshaped, {0, 2, 1, 3});
  mx::array Qk = mx::matmul(QReshaped, KReshaped);

  if (Mask.has_value()) {
    Qk = Qk + slice(*Mask, {0, 0}, {NCtx, NCtx});
  }
  mx::array W = mx::softmax(Qk, /*axis=*/-1, /*precise=*/true);
  mx::array Out = transpose(mx::matmul(W, VReshaped), {0, 2, 1, 3});
  Out = reshape(Out, {NBatch, NCtx, NState});
  return {Out, Qk};
}

// ResidualAttentionBlock implementation
ResidualAttentionBlock::ResidualAttentionBlock(int NState, int NHead,
                                               bool CrossAttention)
    : HasCrossAttention(CrossAttention) {

  auto Attn = std::make_shared<MultiHeadAttention>(NState, NHead);
  auto AttnLn = std::make_shared<nn::LayerNorm>(NState);

  registerModule("attn", Attn);
  registerModule("attn_ln", AttnLn);

  if (CrossAttention) {
    auto CrossAttn = std::make_shared<MultiHeadAttention>(NState, NHead);
    auto CrossAttnLn = std::make_shared<nn::LayerNorm>(NState);
    registerModule("cross_attn", CrossAttn);
    registerModule("cross_attn_ln", CrossAttnLn);
  }

  int NMlp = NState * 4;
  auto Mlp1 = std::make_shared<nn::Linear>(NState, NMlp);
  auto Mlp2 = std::make_shared<nn::Linear>(NMlp, NState);
  auto MlpLn = std::make_shared<nn::LayerNorm>(NState);

  registerModule("mlp1", Mlp1);
  registerModule("mlp2", Mlp2);
  registerModule("mlp_ln", MlpLn);
}

std::tuple<mx::array,
           std::pair<std::optional<std::pair<mx::array, mx::array>>,
                     std::optional<std::pair<mx::array, mx::array>>>,
           std::optional<mx::array>>
ResidualAttentionBlock::forward(
    const mx::array &X, const std::optional<mx::array> &Xa,
    const std::optional<mx::array> &Mask,
    const std::optional<
        std::pair<std::optional<std::pair<mx::array, mx::array>>,
                  std::optional<std::pair<mx::array, mx::array>>>> &KvCache) {

  auto Attn = std::dynamic_pointer_cast<MultiHeadAttention>(Submodules["attn"]);
  auto AttnLn = std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["attn_ln"]);
  auto Mlp1 = std::dynamic_pointer_cast<nn::Linear>(Submodules["mlp1"]);
  auto Mlp2 = std::dynamic_pointer_cast<nn::Linear>(Submodules["mlp2"]);
  auto MlpLn = std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["mlp_ln"]);

  std::optional<std::pair<mx::array, mx::array>> Kv =
      KvCache ? KvCache->first : std::nullopt;
  std::optional<std::pair<mx::array, mx::array>> CrossKv =
      KvCache ? KvCache->second : std::nullopt;
  auto [Y, NewKv, _] =
      Attn->forward(AttnLn->forward(X), std::nullopt, Mask, Kv);
  mx::array Result = X + Y;

  std::optional<mx::array> CrossQk = std::nullopt;
  std::optional<std::pair<mx::array, mx::array>> NewCrossKv = std::nullopt;

  if (HasCrossAttention) {
    auto CrossAttn =
        std::dynamic_pointer_cast<MultiHeadAttention>(Submodules["cross_attn"]);
    auto CrossAttnLn =
        std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["cross_attn_ln"]);

    auto [CrossY, TempCrossKv, TempCrossQk] = CrossAttn->forward(
        CrossAttnLn->forward(Result), Xa, std::nullopt, CrossKv);
    Result = Result + CrossY;
    NewCrossKv = TempCrossKv;
    CrossQk = TempCrossQk;
  }
  Result = Result + Mlp2->forward(
                        mlx::core::gelu(Mlp1->forward(MlpLn->forward(Result))));
  return {Result, {NewKv, NewCrossKv}, CrossQk};
}

// AudioEncoder implementation
AudioEncoder::AudioEncoder(int NMels, int NCtx, int NState, int NHead,
                           int NLayer, mx::Dtype Dtype) {
  auto Conv1 = std::make_shared<nn::Conv1d>(NMels, NState, 3, 1, 1);
  auto Conv2 = std::make_shared<nn::Conv1d>(NState, NState, 3, 2, 1);

  registerModule("conv1", Conv1);
  registerModule("conv2", Conv2);

  PositionalEmbedding = astype(sinusoids(NCtx, NState), Dtype);

  for (int I = 0; I < NLayer; ++I) {
    auto Block = std::make_shared<ResidualAttentionBlock>(NState, NHead);
    Blocks.push_back(Block);
  }
  registerLayer("blocks", Blocks);

  auto LnPost = std::make_shared<nn::LayerNorm>(NState);
  registerModule("ln_post", LnPost);
}

mx::array AudioEncoder::forward(const mx::array &X) {
  auto Conv1 = std::dynamic_pointer_cast<nn::Conv1d>(Submodules["conv1"]);
  auto Conv2 = std::dynamic_pointer_cast<nn::Conv1d>(Submodules["conv2"]);
  auto LnPost = std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["ln_post"]);
  mx::array Result = Conv1->forward(X);
  Result = mlx::core::gelu(Result);
  Result = Conv2->forward(Result);
  Result = mlx::core::gelu(Result);
  assert(Result.shape()[1] == PositionalEmbedding.shape()[0] &&
         Result.shape()[2] == PositionalEmbedding.shape()[1]);

  Result = Result + PositionalEmbedding;
  for (auto &Block : Blocks) {
    auto [NewResult, _, __] = Block->forward(Result);
    Result = NewResult;
  }
  return LnPost->forward(Result);
}

// TextDecoder implementation
TextDecoder::TextDecoder(int NVocab, int NCtx, int NState, int NHead,
                         int NLayer, mx::Dtype Dtype) {
  auto TokenEmbedding = std::make_shared<nn::Embedding>(NVocab, NState);
  registerModule("token_embedding", TokenEmbedding);

  registerParameter("positional_embedding", mx::zeros({NCtx, NState}));

  for (int I = 0; I < NLayer; ++I) {
    auto Block = std::make_shared<ResidualAttentionBlock>(
        NState, NHead, /*cross_attention=*/true);
    Blocks.push_back(Block);
  }
  registerLayer("blocks", Blocks);

  auto Ln = std::make_shared<nn::LayerNorm>(NState);
  registerModule("ln", Ln);

  Mask = astype(nn::MultiHeadAttention::createAdditiveCausalMask(NCtx), Dtype);
}

std::tuple<
    mx::array,
    std::vector<std::pair<std::optional<std::pair<mx::array, mx::array>>,
                          std::optional<std::pair<mx::array, mx::array>>>>,
    std::vector<std::optional<mx::array>>>
TextDecoder::forward(
    const mx::array &X, const mx::array &Xa,
    const std::optional<
        std::vector<std::pair<std::optional<std::pair<mx::array, mx::array>>,
                              std::optional<std::pair<mx::array, mx::array>>>>>
        &KvCache) {

  auto TokenEmbedding =
      std::dynamic_pointer_cast<nn::Embedding>(Submodules["token_embedding"]);
  auto Ln = std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["ln"]);

  int Offset = 0;
  if (KvCache.has_value() && !KvCache->empty() &&
      KvCache->at(0).first.has_value() &&
      KvCache->at(0).first->first.shape(1) > 0) {
    Offset = KvCache->at(0).first->first.shape(1);
  }
  std::vector<int> Start(Parameters.at("positional_embedding").shape().size(),
                         0);
  std::vector<int> End = Parameters.at("positional_embedding").shape();
  Start[0] = Offset;
  End[0] = Offset + X.shape(-1);

  mx::array Result = TokenEmbedding->forward(X) +
                     slice(Parameters.at("positional_embedding"), Start, End);

  std::vector<std::pair<std::optional<std::pair<mx::array, mx::array>>,
                        std::optional<std::pair<mx::array, mx::array>>>>
      NewKvCache;
  std::vector<std::optional<mx::array>> CrossQk;

  if (!KvCache.has_value()) {
    NewKvCache.resize(Blocks.size());
    for (auto &Item : NewKvCache) {
      Item = {std::nullopt, std::nullopt};
    }
  } else {
    NewKvCache = *KvCache;
  }

  CrossQk.resize(Blocks.size());

  for (size_t I = 0; I < Blocks.size(); ++I) {
    auto [NewResult, UpdatedCache, BlockCrossQk] =
        Blocks[I]->forward(Result, Xa, Mask, NewKvCache[I]);
    Result = NewResult;
    NewKvCache[I] = UpdatedCache;
    CrossQk[I] = BlockCrossQk;
  }
  Result = Ln->forward(Result);
  mx::array Logits = TokenEmbedding->asLinear(Result);

  return {Logits, NewKvCache, CrossQk};
}

Whisper::Whisper(const ModelDimensions &Dims, mx::Dtype Dtype) : Dims(Dims) {
  auto Encoder = std::make_shared<AudioEncoder>(
      Dims.NMels, Dims.NAudioCtx, Dims.NAudioState, Dims.NAudioHead,
      Dims.NAudioLayer, Dtype);

  auto Decoder =
      std::make_shared<TextDecoder>(Dims.NVocab, Dims.NTextCtx, Dims.NTextState,
                                    Dims.NTextHead, Dims.NTextLayer, Dtype);

  registerModule("encoder", Encoder);
  registerModule("decoder", Decoder);
  registerParameter("alignment_heads", mx::array({}));
  // // Initialize alignment heads (use last half of decoder layers by default)
  // std::vector<std::vector<bool>> AllHeads(
  //     Dims.NTextLayer, std::vector<bool>(Dims.NTextHead, false));
  // for (int I = Dims.NTextLayer / 2; I < Dims.NTextLayer; ++I) {
  //   std::fill(AllHeads[I].begin(), AllHeads[I].end(), true);
  // }

  // // Find all True positions and create the alignment heads array
  // // Equivalent to: self.alignment_heads =
  // // mx.array(np.asarray(all_heads.nonzero()).T)
  // std::vector<std::vector<int>> NonzeroIndices;
  // for (int Layer = 0; Layer < Dims.NTextLayer; ++Layer) {
  //   for (int Head = 0; Head < Dims.NTextHead; ++Head) {
  //     if (AllHeads[Layer][Head]) {
  //       NonzeroIndices.push_back({Layer, Head});
  //     }
  //   }
  // }

  // // Convert to mx::array format [N, 2] where N is number of True positions
  // if (!NonzeroIndices.empty()) {
  //   std::vector<int> FlatIndices;
  //   FlatIndices.reserve(NonzeroIndices.size() * 2);
  //   for (const auto &Index : NonzeroIndices) {
  //     FlatIndices.push_back(Index[0]); // Layer index
  //     FlatIndices.push_back(Index[1]); // Head index
  //   }
  //   AlignmentHeads =
  //       mx::array(FlatIndices.data(),
  //                 {static_cast<int>(NonzeroIndices.size()), 2}, mx::int64);
  // } else {
  //   // Create empty array with correct shape
  //   AlignmentHeads = mx::zeros({0, 2}, mx::int64);
  // }
}

mx::array Whisper::forward(const mx::array &Mel, const mx::array &Tokens) {
  auto Encoder = std::dynamic_pointer_cast<AudioEncoder>(Submodules["encoder"]);
  auto Decoder = std::dynamic_pointer_cast<TextDecoder>(Submodules["decoder"]);

  mx::array AudioFeatures = Encoder->forward(Mel);
  auto [Logits, _, __] = Decoder->forward(Tokens, AudioFeatures);
  return Logits;
}

mx::array Whisper::embedAudio(const mx::array &Mel) {
  auto Encoder = std::dynamic_pointer_cast<AudioEncoder>(Submodules["encoder"]);
  return Encoder->forward(Mel);
}

mx::array Whisper::logits(const mx::array &Tokens,
                          const mx::array &AudioFeatures) {
  auto Decoder = std::dynamic_pointer_cast<TextDecoder>(Submodules["decoder"]);
  auto [Logits, _, __] = Decoder->forward(Tokens, AudioFeatures);
  return Logits;
}

std::pair<mx::array, std::vector<std::optional<mx::array>>>
Whisper::forwardWithCrossQk(const mx::array &Mel, const mx::array &Tokens) {
  auto Encoder = std::dynamic_pointer_cast<AudioEncoder>(Submodules["encoder"]);
  auto Decoder = std::dynamic_pointer_cast<TextDecoder>(Submodules["decoder"]);

  mx::array AudioFeatures = Encoder->forward(Mel);
  auto [Logits, _, CrossQk] = Decoder->forward(Tokens, AudioFeatures);
  return {Logits, CrossQk};
}

bool Whisper::isMultilingual() const { return Dims.NVocab >= 51865; }

int Whisper::numLanguages() const {
  return Dims.NVocab - 51765 - static_cast<int>(isMultilingual());
}

std::shared_ptr<Whisper> Whisper::fromPretrained(const std::string &ModelPath) {
  std::filesystem::path Path(ModelPath);
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto Error = Parser.load((Path / "config.json").string()).get(Doc);
  if (Error) {
    spdlog::error("Could not open config.json at {}", Path.string());
    assumingUnreachable();
  }
  auto Obj = Doc.get_object();
  ModelDimensions DefaultDims = ModelDimensions::fromDict(Obj.value());
  auto Model = std::make_shared<Whisper>(DefaultDims);
  std::vector<std::filesystem::path> WeightFiles;
  for (auto &P : std::filesystem::directory_iterator(Path)) {
    if (P.path().extension() == ".safetensors")
      WeightFiles.push_back(P.path());
  }
  if (WeightFiles.empty()) {
    spdlog::error("No safetensors found in {}.", Path.string());
    assumingUnreachable();
  }
  std::unordered_map<std::string, mx::array> Weights;
  for (auto &Wf : WeightFiles) {
    auto W = mx::load_safetensors(Wf.string());
    Weights.insert(W.first.begin(), W.first.end());
  }
  Model->update(Weights);

  return Model;
}

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX