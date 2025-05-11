// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "vlm_base.h"
#include "common/errcode.h"
#include "model/vlm_sampling.h"
#include "spdlog/spdlog.h"
#include <ctime>
#include <memory>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <mlx/transforms.h>
#include <stdexcept>
#include <tuple>
#include <vector>
namespace WasmEdge::Host::WASINN::MLX {
namespace vlm {

// BaseCache implementation
std::vector<mx::array> BaseCache::getState() const { return {}; }

void BaseCache::setState(const std::vector<mx::array> &State) {
  if (!State.empty()) {
    spdlog::error(
        "[WASI-NN] MLX backend: This cache has no state but a state was set."sv);
    assumingUnreachable();
  }
}

std::string BaseCache::getMetaState() const { return ""; }

void BaseCache::setMetaState(const std::string &Value) {
  if (!Value.empty()) {
    spdlog::error(
        "[WASI-NN] MLX backend: This cache has no meta_state but a meta_state was set."sv);
    assumingUnreachable();
  }
}

bool BaseCache::isTrimmable() const { return false; }

int BaseCache::trim(int) { return 0; }

// KVCache implementation
KVCache::KVCache(int HeadDim, int NKVHeads, int Step)
    : NKVHeads(NKVHeads), KHeadDim(HeadDim), VHeadDim(HeadDim), Step(Step) {}

KVCache::KVCache(std::pair<int, int> HeadDims, int NKVHeads, int Step)
    : NKVHeads(NKVHeads), KHeadDim(HeadDims.first), VHeadDim(HeadDims.second),
      Step(Step) {}

std::tuple<mx::array, mx::array>
KVCache::updateAndFetch(const mx::array &NewKeys, const mx::array &NewValues) {
  update(NewKeys, NewValues);
  return fetch();
}

std::tuple<mx::array, mx::array> KVCache::fetch() const {
  // self.keys[..., : self.offset, :], self.values[..., : self.offset, :]
  mx::array Indices = mx::arange(Offset);
  mx::array KeysSlice = take(Keys, Indices, -2);
  mx::array ValuesSlice = take(Values, Indices, -2);
  return {KeysSlice, ValuesSlice};
}

void KVCache::update(const mx::array &NewKeys, const mx::array &NewValues) {
  int Prev = Offset;
  std::vector<int> NewShape = NewKeys.shape();
  int NewLen = NewShape[2];

  if (Keys.size() == 0 || (Prev + NewLen) > Keys.shape()[2]) {
    int NSteps = (Step + NewLen - 1) / Step;
    int NewCapacity = NSteps * Step;
    std::vector<int> KShape = {1, NKVHeads, NewCapacity, KHeadDim};
    std::vector<int> VShape = {1, NKVHeads, NewCapacity, VHeadDim};
    mx::array NewK = mx::zeros(KShape, NewKeys.dtype());
    mx::array NewV = mx::zeros(VShape, NewValues.dtype());
    if (Keys.size() != 0) {
      if (Prev % Step != 0) {
        mx::array Indices = mx::arange(Prev);
        Keys = take(Keys, Indices, -2);
        Values = take(Values, Indices, -2);
      }
      Keys = mx::concatenate({Keys, NewK}, 2);
      Values = mx::concatenate({Values, NewV}, 2);
    } else {
      Keys = NewK;
      Values = NewV;
    }
  }

  Offset += NewLen;
  // self.keys[..., prev : self.offset, :] = keys
  // self.values[..., prev : self.offset, :] = values
  auto End = NewKeys.shape();
  std::vector<int> Start(End.size(), 0);
  std::vector<int> Stride(End.size(), 1);
  Start[End.size() - 2] = Prev;
  End[End.size() - 2] = Offset;
  Keys = mx::slice_update(Keys, NewKeys, Start, End, Stride);
  Values = mx::slice_update(Values, NewValues, Start, End, Stride);
}

std::vector<mx::array> KVCache::getState() const {
  if (Offset == Keys.shape()[2]) {
    return {Keys, Values};
  }
  mx::array Indices = mx::arange(Offset);
  mx::array KeysSlice = take(Keys, Indices, -2);
  mx::array ValuesSlice = take(Values, Indices, -2);
  return {KeysSlice, ValuesSlice};
}

void KVCache::setState(const std::vector<mx::array> &State) {
  if (State.size() != 2) {
    spdlog::error(
        "[WASI-NN] MLX backend: KVCache state must contain exactly two arrays"sv);
    assumingUnreachable();
  }
  Keys = State[0];
  Values = State[1];
  Offset = Keys.shape()[2];
}

bool KVCache::isTrimmable() const { return true; }

int KVCache::trim(int N) {
  N = std::min(Offset, N);
  Offset -= N;
  return N;
}

// RotatingKVCache implementation
RotatingKVCache::RotatingKVCache(int MaxSize, int Keep, int Step)
    : KVCache(0, 0, Step), Keep(Keep), MaxSize(MaxSize), Idx(0) {}

mx::array RotatingKVCache::trim(int TrimSize, const mx::array &V,
                                std::optional<mx::array> Append) {
  std::vector<mx::array> ToCat;

  if (TrimSize > 0) {
    mx::array KeepIndices = mx::arange(Keep);
    mx::array TrimIndices = mx::arange(TrimSize + Keep, V.shape()[2]);
    mx::array KeepPart = take(V, KeepIndices, -2);
    mx::array TrimPart = take(V, TrimIndices, -2);
    ToCat = {KeepPart, TrimPart};
  } else {
    ToCat = {V};
  }

  if (Append.has_value()) {
    ToCat.push_back(Append.value());
  }

  return mx::concatenate(ToCat, 2);
}

mx::array RotatingKVCache::temporalOrder(const mx::array &V) {
  if (Idx == V.shape()[2]) {
    return V;
  }
  if (Idx < Offset) {
    mx::array KeepIndices = mx::arange(Keep);
    mx::array IdxToEndIndices = mx::arange(Idx, V.shape()[2]);
    mx::array KeepToIdxIndices = mx::arange(Keep, Idx);

    mx::array KeepPart = take(V, KeepIndices, -2);
    mx::array IdxToEndPart = take(V, IdxToEndIndices, -2);
    mx::array KeepToIdxPart = take(V, KeepToIdxIndices, -2);

    return mx::concatenate({KeepPart, IdxToEndPart, KeepToIdxPart}, 2);
  }
  mx::array IdxIndices = mx::arange(Idx);
  return take(V, IdxIndices, -2);
}

std::tuple<mx::array, mx::array>
RotatingKVCache::updateConcat(const mx::array &NewKeys,
                              const mx::array &NewValues) {
  if (Keys.size() == 0) {
    Keys = NewKeys;
    Values = NewValues;
  } else {
    // Put the keys/values in temporal order to preserve context
    Keys = temporalOrder(Keys);
    Values = temporalOrder(Values);

    // The largest size is MaxSize + S to ensure every token gets at least
    // MaxSize context
    int TrimSize = Idx - MaxSize;
    Keys = trim(TrimSize, Keys, NewKeys);
    Values = trim(TrimSize, Values, NewValues);
  }

  Offset += NewKeys.shape()[2];
  Idx = Keys.shape()[2];
  return {Keys, Values};
}

std::tuple<mx::array, mx::array>
RotatingKVCache::updateInPlace(const mx::array &NewKeys,
                               const mx::array &NewValues) {
  // May not have hit the max size yet, so potentially keep growing the cache
  std::vector<int> KeysShape = NewKeys.shape();
  int B = KeysShape[0];
  int NKVHeads = KeysShape[1];
  int S = KeysShape[2];
  int KHeadDim = KeysShape[3];
  int VHeadDim = NewValues.shape()[3];

  int Prev = Offset;
  if (Keys.size() == 0 ||
      (Prev >= Keys.shape()[2] && Keys.shape()[2] < MaxSize)) {
    int NewSize = std::min(Step, MaxSize - Prev);
    std::vector<int> KShape = {B, NKVHeads, NewSize, KHeadDim};
    std::vector<int> VShape = {B, NKVHeads, NewSize, VHeadDim};

    mx::array NewK = mx::zeros(KShape, NewKeys.dtype());
    mx::array NewV = mx::zeros(VShape, NewValues.dtype());

    if (Keys.size() != 0) {
      Keys = mx::concatenate({Keys, NewK}, 2);
      Values = mx::concatenate({Values, NewV}, 2);
    } else {
      Keys = NewK;
      Values = NewV;
    }
    Idx = Prev;
  }

  // Trim if needed
  int TrimSize = Keys.shape()[2] - MaxSize;
  if (TrimSize > 0) {
    Keys = trim(TrimSize, Keys);
    Values = trim(TrimSize, Values);
    Idx = MaxSize;
  }

  // Rotate
  if (Idx == MaxSize) {
    Idx = Keep;
  }

  // Assign
  std::vector<int> KeysEnd = Keys.shape();
  std::vector<int> Start(KeysEnd.size(), 0);
  std::vector<int> ValuesEnd = Values.shape();
  std::vector<int> Stride(KeysEnd.size(), 1);
  Start[Start.size() - 2] = Idx;
  KeysEnd[KeysEnd.size() - 2] = Idx + S;
  ValuesEnd[KeysEnd.size() - 2] = Idx + S;
  Keys = mx::slice_update(Keys, NewKeys, Start, KeysEnd, Stride);
  Values = mx::slice_update(Values, NewValues, Start, ValuesEnd, Stride);

  Offset += S;
  Idx += S;

  // If the buffer is not full, slice off the end
  if (Offset < MaxSize) {
    mx::array OffsetIndices = mx::arange(Offset);
    mx::array KeysSlice = take(Keys, OffsetIndices, -2);
    mx::array ValuesSlice = take(Values, OffsetIndices, -2);
    return {KeysSlice, ValuesSlice};
  }

  return {Keys, Values};
}

std::tuple<mx::array, mx::array>
RotatingKVCache::updateAndFetch(const mx::array &NewKeys,
                                const mx::array &NewValues) {
  if (NewKeys.shape()[2] == 1) {
    return updateInPlace(NewKeys, NewValues);
  }
  return updateConcat(NewKeys, NewValues);
}

std::string RotatingKVCache::getMetaState() const {
  return std::to_string(Keep) + "," + std::to_string(MaxSize) + "," +
         std::to_string(Step) + "," + std::to_string(Offset) + "," +
         std::to_string(Idx);
}

void RotatingKVCache::setMetaState(const std::string &Value) {
  std::stringstream SS(Value);
  std::string Item;
  std::vector<int> Values;

  while (std::getline(SS, Item, ',')) {
    Values.push_back(std::stoi(Item));
  }

  if (Values.size() == 5) {
    Keep = Values[0];
    MaxSize = Values[1];
    Step = Values[2];
    Offset = Values[3];
    Idx = Values[4];
  } else {
    spdlog::error("[WASI-NN] MLX backend: Invalid meta state format."sv);
    assumingUnreachable();
  }
}

bool RotatingKVCache::isTrimmable() const { return Offset < MaxSize; }

int RotatingKVCache::trim(int N) {
  N = std::min(Offset, N);
  Offset -= N;
  Idx -= N;
  return N;
}

mx::array createAdditiveCausalMask(int N, int Offset) {
  auto Rinds = mx::arange(Offset + N);
  mx::array Linds = mx::array({});
  if (Offset) {
    Linds = mx::arange(Offset, Offset + N);
  } else {
    Linds = Rinds;
  }
  // mask = linds[:, None] < rinds[None]
  return mx::less(mx::expand_dims(Linds, 1), mx::expand_dims(Rinds, 0)) * -1e9;
}

std::optional<mx::array> createAttentionMask(
    mx::array H,
    std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> Cache) {
  int T = H.shape()[1];
  std::optional<mx::array> Mask = std::nullopt;
  if (T > 1) {
    int Offset = 0;
    if (Cache.has_value() && Cache.value().size() > 0 &&
        Cache.value()[0] != nullptr) {
      auto C = Cache.value()[0];
      auto RotCache = std::dynamic_pointer_cast<RotatingKVCache>(C);
      if (RotCache) {
        Offset = std::min(RotCache->MaxSize - 1, RotCache->Offset);
      } else {
        Offset = C->Offset;
      }
    }
    Mask = createAdditiveCausalMask(T, Offset);
    Mask = mx::astype(Mask.value(), H.dtype());
  }
  return Mask;
}

std::vector<int> Module::generate(
    const std::string &Prompt, std::optional<std::string> Image, bool Verbose,
    std::map<std::string, std::variant<mx::array, int, float, std::string>>
        Kwargs) {

  if (Verbose) {
    spdlog::info("=========="sv);
    if (Image.has_value()) {
      spdlog::info("Files: {}\n"sv, Image.value());
    } else if (Kwargs.count("Video") > 0) {
      /* Print video path */
    }
    spdlog::info("Prompt: {}"sv, Prompt);
  }

  std::string Text = "";
  // stream generate
  mx::array InputIds = mx::array({});
  mx::array PixelValues = mx::array({});
  mx::array Mask = mx::array({});

  // For pixel_values
  // int ImageTokenIndex;
  // auto ImageTokenIndexIt = Kwargs.find("image_token_index");
  // if (ImageTokenIndexIt != Kwargs.end()) {
  //   if (auto *ImageTokenIndexPtr =
  //           std::get_if<int>(&ImageTokenIndexIt->second)) {
  //     ImageTokenIndex = *ImageTokenIndexPtr;
  //   } else {
  //     assumingUnreachable();
  //   }
  // } else {
  //   assumingUnreachable();
  // }
  if (Kwargs.count("pixel_values") == 0) {
    spdlog::error("Not implemented");
    assumingUnreachable();
  } else {
    InputIds = *std::get_if<mx::array>(&Kwargs.find("input_ids")->second);
    PixelValues = *std::get_if<mx::array>(&Kwargs.find("pixel_values")->second);
    Mask = *std::get_if<mx::array>(&Kwargs.find("mask")->second);
  }
  // Generate_state
  // Initialize cache
  std::vector<std::shared_ptr<BaseCache>> Cache;
  int MaxTokens = 256;
  float Temperature = 0.0f;
  std::optional<float> RepetitionPenalty = std::nullopt;
  size_t RepetitionContextSize = 20;
  float TopP = 1.0f;
  std::map<int, float> LogitBias = {};
  auto LanguageModel = std::dynamic_pointer_cast<vlm::LanguageModel>(
      this->Submodules["language_model"]);

  auto Sample = [&](mx::array Logits) -> std::tuple<mx::array, mx::array> {
    if (!LogitBias.empty()) {
      for (const auto &[Index, Value] : LogitBias) {
        Logits =
            scatter_add_axis(Logits, mx::array({Index}), mx::array({Value}), 1);
      }
    }

    mx::array LogProbs = Logits - mx::logsumexp(Logits, -1);
    mx::array Token = mx::array({});
    if (Temperature == 0.0f) {
      Token = mx::argmax(Logits, -1);
    } else {
      if (TopP > 0 and TopP < 1.0) {
        Token = topPSampling(Logits, TopP, Temperature);
      } else {
        Token = mx::random::categorical(Logits / Temperature);
      }
    }

    return {Token, LogProbs};
  };

  if (RepetitionPenalty.has_value() && RepetitionPenalty < 0) {
    spdlog::error("Repetition penalty must be greater than 0");
    assumingUnreachable();
  }

  auto MakeCache = LanguageModel->makeCache();
  Cache.insert(Cache.begin(), MakeCache.begin(), MakeCache.end());

  // Initialize repetition context
  auto FlatternInputIdsShape = reshape(InputIds, {-1});
  mx::async_eval(FlatternInputIdsShape);
  std::vector<int> RepetitionContext(FlatternInputIdsShape.data<int>(),
                                     FlatternInputIdsShape.data<int>() +
                                         InputIds.size());
  if (RepetitionContext.size() > RepetitionContextSize) {
    RepetitionContext.erase(RepetitionContext.begin(),
                            RepetitionContext.end() - RepetitionContextSize);
  }

  auto Step = [&](mx::array Y) -> std::tuple<mx::array, mx::array> {
    std::vector<int> NewShape = Y.shape();
    NewShape.insert(NewShape.begin(), 1);
    // TODO: handle decoder_input_ids
    auto Outputs = std::dynamic_pointer_cast<vlm::LanguageModel>(
                       this->Submodules["language_model"])
                       ->forward(reshape(Y, NewShape), Cache);
    mx::array Logits = std::get<0>(Outputs);
    Logits = take(Logits, Logits.shape()[1] - 1, 1);
    mx::array LogProbs = mx::array({});
    if (RepetitionPenalty.has_value()) {
      if (RepetitionContext.size() > 0) {
        auto Indices = mx::array(RepetitionContext.data(),
                                 {static_cast<int>(RepetitionContext.size())});
        auto SelectedLogits = take(Logits, Indices, 1);
        SelectedLogits = where(SelectedLogits < 0,
                               SelectedLogits * RepetitionPenalty.value(),
                               SelectedLogits / RepetitionPenalty.value());
        put_along_axis(Logits, Indices, SelectedLogits, 1);
      }
      std::tie(Y, LogProbs) = Sample(Logits);
      RepetitionContext.emplace_back(Y.item<int>());
    } else {
      std::tie(Y, LogProbs) = Sample(Logits);
    }
    if (RepetitionContext.size() > RepetitionContextSize) {
      RepetitionContext.erase(RepetitionContext.begin(),
                              RepetitionContext.end() - RepetitionContextSize);
    }
    return {Y, squeeze(LogProbs, 0)};
  };

  // Perform the first step
  auto Outputs = this->forward(InputIds, PixelValues, Mask, Cache);
  mx::array Logits = std::get<0>(Outputs);
  Logits = take(Logits, Logits.shape()[1] - 1, 1);
  auto [Y, LogProbs] = Sample(Logits);
  mx::async_eval(Y);
  // TODO: handle cross_attention_states, encoder_outputs
  // End generate_state

  std::optional<GenerationResult> Result;
  GenerationResult LastResponse;
  // auto Tic = std::chrono::system_clock::now().time_since_epoch();
  std::vector<int32_t> TokenList;
  int N = 0;
  while (true) {
    // for statistic
    // int PromptTPS;
    // if (N == 0) {
    //   PromptTPS =
    //   std::chrono::duration_cast<std::chrono::seconds>(Tic).count(); Tic =
    //   std::chrono::system_clock::now().time_since_epoch();
    // }
    // TODO: if token = eos_token_id break
    auto Response = GenerationResult();
    N++;
    if (N >= MaxTokens) {
      break;
    }
    auto [NextY, NextLogProbs] = Step(Y);
    mx::async_eval(NextY);
    // TODO: handle decoder_input_ids
    auto Token = Y.item<int>();
    if (Token == 1 || Token == 106) {
      break;
    }
    TokenList.emplace_back(Token);
    Y = NextY;
    LogProbs = NextLogProbs;
  }
  return TokenList;
  // end stream generate
  // TODO: waiting for processor
  if (Verbose) {
    spdlog::info("\n=========="sv);
    if (Text.empty()) {
      spdlog::info("No text generated for this prompt"sv);
      return {};
      // return Text;
    }

    spdlog::info("Prompt: {} tokends, {} tokens-per-sec"sv,
                 LastResponse.PromptTokens, LastResponse.PromptTps);
    spdlog::info("Generation: {} tokens {} tokens-per-sec"sv,
                 LastResponse.GenerationTokens, LastResponse.GenerationTps);
    spdlog::info("Peak memory: {} GB"sv);
  }

  // return Text;
}
} // namespace vlm

} // namespace WasmEdge::Host::WASINN::MLX
