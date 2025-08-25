// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "mlx/base.h"
#include "simdjson.h"
#include <memory>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {

namespace nn = mlx::core::nn;

namespace whisper {

struct ModelDimensions {
  int NMels = 80;
  int NAudioCtx = 1500;
  int NAudioState = 768;
  int NAudioHead = 12;
  int NAudioLayer = 12;
  int NVocab = 51864;
  int NTextCtx = 448;
  int NTextState = 768;
  int NTextHead = 12;
  int NTextLayer = 12;

  static ModelDimensions fromDict(const simdjson::dom::object &Obj);
};

// Utility function for positional embeddings
mx::array sinusoids(int Length, int Channels, float MaxTimescale = 10000.0f);

class MultiHeadAttention : public nn::Module {
public:
  MultiHeadAttention(int NState, int NHead);

  std::tuple<mx::array, std::pair<mx::array, mx::array>, mx::array>
  forward(const mx::array &X, const std::optional<mx::array> &Xa = std::nullopt,
          const std::optional<mx::array> &Mask = std::nullopt,
          const std::optional<std::pair<mx::array, mx::array>> &KvCache =
              std::nullopt);

private:
  std::pair<mx::array, mx::array>
  qkvAttention(const mx::array &Q, const mx::array &K, const mx::array &V,
               const std::optional<mx::array> &Mask = std::nullopt);

  int NHead;
};

class ResidualAttentionBlock : public nn::Module {
public:
  ResidualAttentionBlock(int NState, int NHead, bool CrossAttention = false);

  std::tuple<mx::array,
             std::pair<std::optional<std::pair<mx::array, mx::array>>,
                       std::optional<std::pair<mx::array, mx::array>>>,
             std::optional<mx::array>>
  forward(const mx::array &X, const std::optional<mx::array> &Xa = std::nullopt,
          const std::optional<mx::array> &Mask = std::nullopt,
          const std::optional<
              std::pair<std::optional<std::pair<mx::array, mx::array>>,
                        std::optional<std::pair<mx::array, mx::array>>>>
              &KvCache = std::nullopt);

private:
  bool HasCrossAttention;
};

class AudioEncoder : public nn::Module {
public:
  AudioEncoder(int NMels, int NCtx, int NState, int NHead, int NLayer,
               mx::Dtype Dtype = mx::float16);

  mx::array forward(const mx::array &X);

private:
  mx::array PositionalEmbedding = mx::array({});
  std::vector<std::shared_ptr<ResidualAttentionBlock>> Blocks;
};

class TextDecoder : public nn::Module {
public:
  TextDecoder(int NVocab, int NCtx, int NState, int NHead, int NLayer,
              mx::Dtype Dtype = mx::float16);

  std::tuple<
      mx::array,
      std::vector<std::pair<std::optional<std::pair<mx::array, mx::array>>,
                            std::optional<std::pair<mx::array, mx::array>>>>,
      std::vector<std::optional<mx::array>>>
  forward(const mx::array &X, const mx::array &Xa,
          const std::optional<std::vector<
              std::pair<std::optional<std::pair<mx::array, mx::array>>,
                        std::optional<std::pair<mx::array, mx::array>>>>>
              &KvCache = std::nullopt);

private:
  mx::array Mask = mx::array({});
  std::vector<std::shared_ptr<ResidualAttentionBlock>> Blocks;
};

class Whisper : public nn::Module {
public:
  Whisper(const ModelDimensions &Dims, mx::Dtype Dtype = mx::float16);

  mx::array forward(const mx::array &Mel, const mx::array &Tokens);

  mx::array embedAudio(const mx::array &Mel);
  mx::array logits(const mx::array &Tokens, const mx::array &AudioFeatures);
  std::pair<mx::array, std::vector<std::optional<mx::array>>>
  forwardWithCrossQk(const mx::array &Mel, const mx::array &Tokens);

  bool isMultilingual() const;
  int numLanguages() const;

  static std::shared_ptr<Whisper> fromPretrained(const std::string &ModelPath);

  ModelDimensions Dims;
  mx::array AlignmentHeads = mx::array({});
};

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX