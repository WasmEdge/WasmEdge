// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "whisper.h"
#include <map>
#include <memory>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace whisper {

class Tokenizer;
class Whisper;

float compressionRatio(const std::string &Text);

std::pair<mx::array, std::vector<std::map<std::string, float>>>
detectLanguage(std::shared_ptr<Whisper> Model, const mx::array &Mel,
               std::shared_ptr<Tokenizer> Tokenizer = nullptr);

struct DecodingOptions {
  std::string Task = "transcribe";
  std::optional<std::string> Language = std::nullopt;
  float Temperature = 0.0f;
  std::optional<int> SampleLen = std::nullopt;
  std::optional<int> BestOf = std::nullopt;
  std::optional<int> BeamSize = std::nullopt;
  std::optional<float> Patience = std::nullopt;
  std::optional<float> LengthPenalty = std::nullopt;
  std::optional<std::variant<std::string, std::vector<int>>> Prompt =
      std::nullopt;
  std::optional<std::variant<std::string, std::vector<int>>> Prefix =
      std::nullopt;
  std::optional<std::variant<std::string, std::vector<int>>> SuppressTokens =
      "-1";
  bool SuppressBlank = true;
  bool WithoutTimestamps = false;
  std::optional<float> MaxInitialTimestamp = 1.0f;
  bool Fp16 = true;
};

struct DecodingResult {
  mx::array AudioFeatures;
  std::string Language;
  std::optional<std::map<std::string, float>> LanguageProbs = std::nullopt;
  std::vector<int> Tokens;
  std::string Text = "";
  float AvgLogprob = std::numeric_limits<float>::quiet_NaN();
  float NoSpeechProb = std::numeric_limits<float>::quiet_NaN();
  float Temperature = std::numeric_limits<float>::quiet_NaN();
  float CompressionRatio = std::numeric_limits<float>::quiet_NaN();
  DecodingResult() : AudioFeatures(mx::array({})) {}
};

class Inference {
public:
  explicit Inference(std::shared_ptr<Whisper> Model);
  mx::array logits(const mx::array &Tokens, const mx::array &AudioFeatures);
  void rearrangeKvCache(const std::vector<int> &SourceIndices);
  void reset();

private:
  std::shared_ptr<Whisper> Model;
  std::optional<
      std::vector<std::pair<std::optional<std::pair<mx::array, mx::array>>,
                            std::optional<std::pair<mx::array, mx::array>>>>>
      KvCache = std::nullopt;
};

class SequenceRanker {
public:
  virtual ~SequenceRanker() = default;
  virtual std::vector<int>
  rank(const std::vector<std::vector<std::vector<int>>> &Tokens,
       const std::vector<std::vector<float>> &SumLogprobs) = 0;
};

class MaximumLikelihoodRanker : public SequenceRanker {
public:
  explicit MaximumLikelihoodRanker(std::optional<float> LengthPenalty);
  std::vector<int>
  rank(const std::vector<std::vector<std::vector<int>>> &Tokens,
       const std::vector<std::vector<float>> &SumLogprobs) override;

private:
  std::optional<float> LengthPenalty;
};

class TokenDecoder {
public:
  virtual ~TokenDecoder() = default;
  virtual void reset() = 0;
  virtual std::tuple<mx::array, bool, mx::array>
  update(const mx::array &Tokens, const mx::array &Logits,
         const mx::array &SumLogprobs) = 0;
  virtual std::pair<mx::array, mx::array>
  finalize(const mx::array &Tokens, const mx::array &SumLogprobs) = 0;
};

class GreedyDecoder : public TokenDecoder {
public:
  GreedyDecoder(float Temperature, int Eot);
  void reset() override;
  std::tuple<mx::array, bool, mx::array>
  update(const mx::array &Tokens, const mx::array &Logits,
         const mx::array &SumLogprobs) override;
  std::pair<mx::array, mx::array>
  finalize(const mx::array &Tokens, const mx::array &SumLogprobs) override;

private:
  float Temperature;
  int Eot;
};

class LogitFilter {
public:
  virtual ~LogitFilter() = default;
  virtual mx::array apply(const mx::array &Logits, const mx::array &Tokens) = 0;
  std::string Name = "LogitFilter";
};

class SuppressBlank : public LogitFilter {
public:
  SuppressBlank(std::shared_ptr<Tokenizer> Tokenizer, int SampleBegin,
                int NVocab);
  mx::array apply(const mx::array &Logits, const mx::array &Tokens) override;

private:
  int SampleBegin;
  mx::array Mask;
};

class SuppressTokens : public LogitFilter {
public:
  SuppressTokens(const std::vector<int> &SuppressTokens, int NVocab);
  mx::array apply(const mx::array &Logits, const mx::array &Tokens) override;

private:
  mx::array Mask;
};

class ApplyTimestampRules : public LogitFilter {
public:
  ApplyTimestampRules(std::shared_ptr<Tokenizer> Tokenizer, int SampleBegin,
                      std::optional<int> MaxInitialTimestampIndex);
  mx::array apply(const mx::array &Logits, const mx::array &Tokens) override;

private:
  std::shared_ptr<Tokenizer> Tokenizer;
  int SampleBegin;
  std::optional<int> MaxInitialTimestampIndex;
};

class DecodingTask {
public:
  DecodingTask(std::shared_ptr<Whisper> Model, const DecodingOptions &Options);
  std::vector<DecodingResult> run(const mx::array &Mel);

private:
  DecodingOptions verifyOptions(const DecodingOptions &Options);
  std::vector<int> getInitialTokens();
  std::vector<int> getSuppressTokens();
  mx::array getAudioFeatures(const mx::array &Mel);
  std::pair<std::vector<std::string>,
            std::optional<std::vector<std::map<std::string, float>>>>
  detectLanguage(const mx::array &AudioFeatures, mx::array &Tokens);
  std::tuple<mx::array, mx::array, mx::array>
  mainLoop(const mx::array &AudioFeatures, const mx::array &Tokens);
  std::shared_ptr<Whisper> Model;
  std::shared_ptr<Tokenizer> Tokenizer;
  DecodingOptions Options;
  int NGroup;
  int NCtx;
  int SampleLen;
  std::vector<int> SotSequence;
  std::vector<int> InitialTokens;
  int SampleBegin;
  int SotIndex;
  std::unique_ptr<Inference> Inference;
  std::unique_ptr<SequenceRanker> SequenceRanker;
  std::unique_ptr<TokenDecoder> Decoder;
  std::vector<std::unique_ptr<LogitFilter>> LogitFilters;
};

std::variant<DecodingResult, std::vector<DecodingResult>>
decode(std::shared_ptr<Whisper> Model, const mx::array &Mel,
       const DecodingOptions &Options = DecodingOptions());

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX
