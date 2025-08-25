// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "decoding.h"
#include "mlx/base.h"
#include "tokenizer.h"
#include "whisper.h"
#include <algorithm>
#include <cassert>
#include <mlx/array.h>
#include <mlx/dtype.h>
#include <mlx/ops.h>
#include <sstream>
#include <zlib.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace whisper {

// Utility function implementation
float compressionRatio(const std::string &Text) {
  if (Text.empty())
    return 1.0f;

  std::vector<uint8_t> Input(Text.begin(), Text.end());
  uLongf CompressedSize = compressBound(Input.size());
  std::vector<uint8_t> Compressed(CompressedSize);

  int Result =
      compress(Compressed.data(), &CompressedSize, Input.data(), Input.size());

  if (Result != Z_OK)
    return 1.0f;

  return static_cast<float>(Input.size()) / static_cast<float>(CompressedSize);
}

// Language detection implementation
std::pair<mx::array, std::vector<std::map<std::string, float>>>
detectLanguage(std::shared_ptr<Whisper> Model, const mx::array &Mel,
               std::shared_ptr<Tokenizer> Tokenizer) {

  if (!Tokenizer) {
    Tokenizer = getTokenizer(Model->isMultilingual(), Model->numLanguages());
  }

  if (!Tokenizer->Language ||
      std::find(Tokenizer->SotSequence.begin(), Tokenizer->SotSequence.end(),
                Tokenizer->languageToken()) == Tokenizer->SotSequence.end()) {
    throw std::runtime_error(
        "This model doesn't have language tokens so it can't perform lang id");
  }
  bool Single = Mel.ndim() == 2;
  mx::array MelArray = Single ? mx::expand_dims(Mel, 0) : Mel;
  // Skip encoder forward pass if already-encoded audio features were given
  if (MelArray.shape(-2) != Model->Dims.NAudioCtx ||
      MelArray.shape(-1) != Model->Dims.NAudioState) {
    MelArray = Model->embedAudio(MelArray);
  }
  // Forward pass using a single token, start of transcript
  int NAudio = MelArray.shape(0);
  std::vector<std::vector<int>> TokensVec(NAudio, {Tokenizer->getSot()});
  mx::array Tokens = mx::array(TokensVec[0].data(), {NAudio, 1}, mx::int32);

  mx::array Logits = Model->logits(Tokens, MelArray);
  Logits = mx::take(Logits, mx::array({0}), 1); // [:, 0]
  // Collect detected languages; suppress all non-language tokens
  mx::array MaskArray = mx::full(
      {Logits.shape(-1)}, -std::numeric_limits<float>::infinity(), mx::float32);
  auto LangTokens = Tokenizer->getAllLanguageTokens();
  mx::array LangTokensArray = mx::array(
      LangTokens.data(), {static_cast<int>(LangTokens.size())}, mx::int32);
  MaskArray =
      mx::scatter(MaskArray, LangTokensArray,
                  mx::zeros({static_cast<int>(LangTokens.size()), 1}), 0);

  Logits = Logits + MaskArray;
  mx::array LanguageTokens = mx::argmax(Logits, -1);
  mx::array LanguageTokenProbs = mx::softmax(Logits, -1);
  LanguageTokenProbs = mx::take(LanguageTokenProbs, 0, 0);

  std::vector<std::map<std::string, float>> LanguageProbs;
  auto LangCodes = Tokenizer->getAllLanguageCodes();

  for (int I = 0; I < NAudio; ++I) {
    std::map<std::string, float> Probs;
    for (size_t J = 0; J < LangTokens.size() && J < LangCodes.size(); ++J) {
      int TokenId = LangTokens[J];
      std::string LangCode = LangCodes[J];

      mx::array ProbArray =
          mx::take(mx::take(LanguageTokenProbs, mx::array({0}), 0),
                   mx::array({TokenId}), -1);
      ProbArray = mx::squeeze(ProbArray);

      float Prob = ProbArray.item<float>();
      Probs[LangCode] = Prob;
    }
    LanguageProbs.push_back(Probs);
  }

  if (Single) {
    LanguageTokens = mx::take(LanguageTokens, mx::array({0}), 0);
    LanguageProbs = {LanguageProbs[0]};
  }

  return {LanguageTokens, LanguageProbs};
}

// Inference class implementation
Inference::Inference(std::shared_ptr<Whisper> Model) : Model(Model) { reset(); }

mx::array Inference::logits(const mx::array &Tokens,
                            const mx::array &AudioFeatures) {
  auto [LogitsOutput, NewKvCache, _] =
      std::dynamic_pointer_cast<TextDecoder>(Model->Submodules.at("decoder"))
          ->forward(Tokens, AudioFeatures, KvCache);
  KvCache = NewKvCache;
  return mx::astype(LogitsOutput, mx::float32);
}

void Inference::rearrangeKvCache(const std::vector<int> &SourceIndices) {
  // TODO: Implement KV cache rearrangement for beam search
  assumingUnreachable();
}

void Inference::reset() { KvCache = std::nullopt; }

// GreedyDecoder implementation
GreedyDecoder::GreedyDecoder(float Temperature, int Eot)
    : Temperature(Temperature), Eot(Eot) {}

void GreedyDecoder::reset() {
  // Nothing to reset for greedy decoder
}

std::tuple<mx::array, bool, mx::array>
GreedyDecoder::update(const mx::array &Tokens, const mx::array &Logits,
                      const mx::array &SumLogprobs) {

  // Sample next tokens
  mx::array NextTokens = mx::array({});
  if (Temperature == 0.0f) {
    NextTokens = mx::argmax(Logits, -1);
  } else {
    NextTokens = mx::random::categorical(Logits / Temperature);
  }

  // Compute logprobs
  mx::array Logprobs = Logits - mx::logsumexp(Logits, -1, false);
  mx::array CurrentLogprobs =
      mx::take(Logprobs, mx::arange(Logprobs.shape(0)), 0);
  CurrentLogprobs = take(CurrentLogprobs, NextTokens, 1);
  mx::array NewSumLogprobs =
      SumLogprobs +
      CurrentLogprobs * (take(Tokens, Tokens.shape(1) - 1, 1) != Eot);

  // Extend tokens
  mx::array NewTokens =
      mx::concatenate({Tokens, mx::expand_dims(NextTokens, -1)}, -1);

  // Check if all sequences are complete
  bool Completed = mx::all(mx::equal(mx::take(NewTokens, mx::array({-1}), 1),
                                     mx::array({Eot})))
                       .item<bool>();

  return {NewTokens, Completed, NewSumLogprobs};
}

std::pair<mx::array, mx::array>
GreedyDecoder::finalize(const mx::array &Tokens, const mx::array &SumLogprobs) {

  // Make sure each sequence has at least one EOT token at the end
  std::vector<std::pair<int, int>> PadWidths = {{0, 0}, {0, 0}, {0, 1}};
  mx::array PaddedTokens =
      mx::pad(Tokens, PadWidths, mx::array(Eot, mx::int32));

  return {PaddedTokens, SumLogprobs};
}

// SuppressBlank implementation
SuppressBlank::SuppressBlank(std::shared_ptr<whisper::Tokenizer> Tokenizer,
                             int SampleBegin, int NVocab)
    : SampleBegin(SampleBegin), Mask(mx::zeros({NVocab}, mx::float32)) {
  Name = "SuppressBlank";
  std::vector<float> MaskVec(NVocab, 0.0f);

  // Suppress space and EOT tokens
  auto SpaceTokens = Tokenizer->encode(" ");
  for (int Token : SpaceTokens) {
    if (Token >= 0 && Token < NVocab) {
      MaskVec[Token] = -std::numeric_limits<float>::infinity();
    }
  }

  int EotToken = Tokenizer->getEot();
  if (EotToken < NVocab) {
    MaskVec[EotToken] = -std::numeric_limits<float>::infinity();
  }

  Mask = mx::array(MaskVec.data(), {NVocab}, mx::float32);
}

mx::array SuppressBlank::apply(const mx::array &Logits,
                               const mx::array &Tokens) {
  if (Tokens.shape(1) == SampleBegin) {
    return Logits + Mask;
  }
  return Logits;
}

// SuppressTokens implementation
SuppressTokens::SuppressTokens(const std::vector<int> &SuppressTokens,
                               int NVocab)
    : Mask(mx::zeros({NVocab}, mx::float32)) {
  Name = "SuppressTokens";
  std::vector<float> MaskVec(NVocab, 0.0f);
  for (int Token : SuppressTokens) {
    MaskVec[Token] = -std::numeric_limits<float>::infinity();
  }

  Mask = mx::array(MaskVec.data(), {NVocab}, mx::float32);
}

mx::array SuppressTokens::apply(const mx::array &Logits,
                                const mx::array &Tokens) {
  return Logits + Mask;
}

// ApplyTimestampRules implementation
ApplyTimestampRules::ApplyTimestampRules(
    std::shared_ptr<whisper::Tokenizer> Tokenizer, int SampleBegin,
    std::optional<int> MaxInitialTimestampIndex)
    : Tokenizer(Tokenizer), SampleBegin(SampleBegin),
      MaxInitialTimestampIndex(MaxInitialTimestampIndex) {
  Name = "ApplyTimestampRules";
}

mx::array ApplyTimestampRules::apply(const mx::array &Logits,
                                     const mx::array &Tokens) {
  auto LogitsShape = Logits.shape();
  std::vector<float> MaskVec(Logits.size(), 0.0f);

  // Suppress <|notimestamps|> which is handled by without_timestamps
  if (Tokenizer->getNoTimestamps()) {
    for (int I = 0; I < LogitsShape[0]; ++I) {
      MaskVec[I * LogitsShape[1] + Tokenizer->getNoTimestamps()] =
          -std::numeric_limits<float>::infinity();
    }
  }

  mx::eval(Tokens);
  std::vector<std::vector<int>> TokensList(Tokens.shape(0));
  for (int K = 0; K < Tokens.shape(0); ++K) {
    TokensList[K].resize(Tokens.shape(1));
    for (int J = 0; J < Tokens.shape(1); ++J) {
      mx::array TokenVal = mx::take(mx::take(Tokens, K, 0), J, 0);
      TokensList[K][J] = TokenVal.item<int>();
    }
  }

  // Timestamps have to appear in pairs, except directly before EOT; mask logits
  // accordingly
  for (int K = 0; K < static_cast<int>(TokensList.size()); ++K) {
    // seq = tokens[k][self.sample_begin :]
    std::vector<int> Seq(TokensList[K].begin() + SampleBegin,
                         TokensList[K].end());

    bool LastWasTimestamp =
        Seq.size() >= 1 &&
        Seq[Seq.size() - 1] >= Tokenizer->getTimestampBegin();
    bool PenultimateWasTimestamp =
        Seq.size() < 2 || Seq[Seq.size() - 2] >= Tokenizer->getTimestampBegin();

    if (LastWasTimestamp) {
      if (PenultimateWasTimestamp) {
        // Has to be non-timestamp
        for (int I = Tokenizer->getTimestampBegin(); I < LogitsShape[1]; ++I) {
          MaskVec[K * LogitsShape[1] + I] =
              -std::numeric_limits<float>::infinity();
        }
      } else {
        // Cannot be normal text tokens
        for (int I = 0; I < Tokenizer->getEot(); ++I) {
          MaskVec[K * LogitsShape[1] + I] =
              -std::numeric_limits<float>::infinity();
        }
      }
    }

    // Find timestamps in sequence and enforce monotonicity
    std::vector<int> Timestamps;
    for (size_t I = 0; I < Seq.size(); ++I) {
      if (Seq[I] > Tokenizer->getTimestampBegin()) {
        Timestamps.push_back(Seq[I]);
      }
    }

    if (!Timestamps.empty()) {
      // Timestamps shouldn't decrease; forbid timestamp tokens smaller than the
      // last Also force each segment to have a nonzero length, to prevent
      // infinite looping
      int LastTimestamp = Timestamps.back();
      if (LastTimestamp == 0 || PenultimateWasTimestamp) {
        LastTimestamp += 1;
      }
      for (int I = Tokenizer->getTimestampBegin(); I < LastTimestamp; ++I) {
        MaskVec[K * LogitsShape[1] + I] =
            -std::numeric_limits<float>::infinity();
      }
    }
  }

  if (static_cast<int>(TokensList[0].size()) == SampleBegin) {
    // Suppress generating non-timestamp tokens at the beginning
    for (int I = 0; I < LogitsShape[0]; ++I) {
      for (int J = 0; J < Tokenizer->getTimestampBegin(); ++J) {
        MaskVec[I * LogitsShape[1] + J] =
            -std::numeric_limits<float>::infinity();
      }
    }

    // Apply the `max_initial_timestamp` option
    if (MaxInitialTimestampIndex) {
      int LastAllowed =
          Tokenizer->getTimestampBegin() + *MaxInitialTimestampIndex;
      for (int I = 0; I < LogitsShape[0]; ++I) {
        for (int J = LastAllowed + 1; J < LogitsShape[1]; ++J) {
          MaskVec[I * LogitsShape[1] + J] =
              -std::numeric_limits<float>::infinity();
        }
      }
    }
  }

  // If sum of probability over timestamps is above any other token, sample
  // timestamp
  mx::array MaskArray = mx::array(MaskVec.data(), LogitsShape, mx::float32);
  mx::array Logprobs = Logits - mx::logsumexp(Logits, -1, true);

  // Calculate timestamp logprob: sum of probabilities for all timestamp tokens
  mx::array TimestampLogprob =
      mx::logsumexp(mx::slice(Logprobs, {0, Tokenizer->getTimestampBegin()},
                              {LogitsShape[0], LogitsShape[1]}),
                    -1, true);

  // Calculate max text token logprob: max probability among non-timestamp
  // tokens
  mx::array MaxTextTokenLogprob =
      mx::max(mx::slice(Logprobs, {0, 0},
                        {LogitsShape[0], Tokenizer->getTimestampBegin()}),
              -1, true);

  // Where timestamp probability > max text probability, suppress text tokens
  mx::array TimestampCondition =
      mx::greater(TimestampLogprob, MaxTextTokenLogprob);

  for (int I = 0; I < LogitsShape[0]; ++I) {
    bool ShouldSuppressText = mx::take(TimestampCondition, I, 0).item<bool>();
    if (ShouldSuppressText) {
      for (int J = 0; J < Tokenizer->getTimestampBegin(); ++J) {
        MaskVec[I * LogitsShape[1] + J] =
            -std::numeric_limits<float>::infinity();
      }
    }
  }

  MaskArray = mx::array(MaskVec.data(), LogitsShape, mx::float32);
  return Logits + MaskArray;
}

// MaximumLikelihoodRanker implementation
MaximumLikelihoodRanker::MaximumLikelihoodRanker(
    std::optional<float> LengthPenalty)
    : LengthPenalty(LengthPenalty) {}

std::vector<int> MaximumLikelihoodRanker::rank(
    const std::vector<std::vector<std::vector<int>>> &Tokens,
    const std::vector<std::vector<float>> &SumLogprobs) {

  std::vector<int> Selected;

  for (size_t I = 0; I < Tokens.size(); ++I) {
    std::vector<float> Scores;

    for (size_t J = 0; J < Tokens[I].size(); ++J) {
      int Length = Tokens[I][J].size();
      float Logprob = SumLogprobs[I][J];

      float Penalty;
      if (LengthPenalty) {
        Penalty = std::pow(Length, *LengthPenalty);
      } else {
        Penalty = Length;
      }

      Scores.push_back(Logprob / Penalty);
    }

    auto MaxIterator = std::max_element(Scores.begin(), Scores.end());
    Selected.push_back(std::distance(Scores.begin(), MaxIterator));
  }

  return Selected;
}

// DecodingTask implementation - Constructor and helper methods
DecodingTask::DecodingTask(std::shared_ptr<Whisper> Model,
                           const DecodingOptions &Options)
    : Model(Model), Options(verifyOptions(Options)) {

  std::string Language = Options.Language.value_or("en");
  Tokenizer = whisper::getTokenizer(
      Model->isMultilingual(), Model->numLanguages(), Language, Options.Task);

  NGroup = Options.BeamSize.value_or(Options.BestOf.value_or(1));
  NCtx = Model->Dims.NTextCtx;
  SampleLen = Options.SampleLen.value_or(NCtx / 2);

  // Handle SOT sequence with without_timestamps logic
  SotSequence = Tokenizer->SotSequence;
  if (Options.WithoutTimestamps) {
    SotSequence = Tokenizer->getSotSequenceIncludingNotimestamps();
  }

  InitialTokens = getInitialTokens();
  SampleBegin = InitialTokens.size();

  // Find SOT index
  auto SotToken = Tokenizer->getSot();
  auto Iterator =
      std::find(InitialTokens.begin(), InitialTokens.end(), SotToken);
  SotIndex = std::distance(InitialTokens.begin(), Iterator);

  // Initialize components
  Inference = std::make_unique<whisper::Inference>(Model);
  SequenceRanker =
      std::make_unique<MaximumLikelihoodRanker>(Options.LengthPenalty);

  if (Options.BeamSize && *Options.BeamSize > 1) {
    throw std::runtime_error("Beam search decoder is not yet implemented");
  }
  Decoder =
      std::make_unique<GreedyDecoder>(Options.Temperature, Tokenizer->getEot());

  LogitFilters.clear();

  if (Options.SuppressBlank) {
    LogitFilters.push_back(std::make_unique<SuppressBlank>(
        Tokenizer, SampleBegin, Model->Dims.NVocab));
  }

  if (Options.SuppressTokens) {
    auto SuppressTokens = getSuppressTokens();
    LogitFilters.push_back(std::make_unique<whisper::SuppressTokens>(
        SuppressTokens, Model->Dims.NVocab));
  }

  if (!Options.WithoutTimestamps) {
    std::optional<int> MaxInitialTimestampIndex;
    if (Options.MaxInitialTimestamp) {
      float Precision =
          30.0f / Model->Dims.NAudioCtx; // CHUNK_LENGTH / n_audio_ctx
      MaxInitialTimestampIndex = static_cast<int>(
          std::round(*Options.MaxInitialTimestamp / Precision));
    }
    LogitFilters.push_back(std::make_unique<ApplyTimestampRules>(
        Tokenizer, SampleBegin, MaxInitialTimestampIndex));
  }
}

DecodingOptions DecodingTask::verifyOptions(const DecodingOptions &Options) {
  DecodingOptions Result = Options;

  // Check beam_size and best_of conflicts
  if (Result.BeamSize && Result.BestOf) {
    throw std::runtime_error("beam_size and best_of can't be given together");
  }

  // Check temperature = 0 with best_of
  if (Result.Temperature == 0.0f && Result.BestOf) {
    throw std::runtime_error(
        "best_of with greedy sampling (T=0) is not compatible");
  }

  // Check patience requires beam_size
  if (Result.Patience && !Result.BeamSize) {
    throw std::runtime_error("patience requires beam_size to be given");
  }

  // Check length_penalty range
  if (Result.LengthPenalty &&
      (*Result.LengthPenalty < 0.0f || *Result.LengthPenalty > 1.0f)) {
    throw std::runtime_error(
        "length_penalty (alpha) should be a value between 0 and 1");
  }

  return Result;
}

std::vector<int> DecodingTask::getInitialTokens() {
  std::vector<int> Tokens = SotSequence;

  if (Options.Prefix) {
    std::vector<int> PrefixTokens;
    if (std::holds_alternative<std::string>(*Options.Prefix)) {
      std::string PrefixStr = std::get<std::string>(*Options.Prefix);
      PrefixTokens = Tokenizer->encode(" " + PrefixStr);
    } else {
      PrefixTokens = std::get<std::vector<int>>(*Options.Prefix);
    }

    if (SampleLen > 0) {
      int MaxPrefixLen = NCtx / 2 - SampleLen;
      if (static_cast<int>(PrefixTokens.size()) > MaxPrefixLen) {
        PrefixTokens = std::vector<int>(PrefixTokens.end() - MaxPrefixLen,
                                        PrefixTokens.end());
      }
    }

    Tokens.insert(Tokens.end(), PrefixTokens.begin(), PrefixTokens.end());
  }

  if (Options.Prompt) {
    std::vector<int> PromptTokens;
    if (std::holds_alternative<std::string>(*Options.Prompt)) {
      std::string PromptStr = std::get<std::string>(*Options.Prompt);
      PromptTokens = Tokenizer->encode(" " + PromptStr);
    } else {
      PromptTokens = std::get<std::vector<int>>(*Options.Prompt);
    }

    int MaxPromptLen = NCtx / 2 - 1;
    if (static_cast<int>(PromptTokens.size()) > MaxPromptLen) {
      PromptTokens = std::vector<int>(PromptTokens.end() - MaxPromptLen,
                                      PromptTokens.end());
    }

    std::vector<int> NewTokens;
    NewTokens.push_back(Tokenizer->getSotPrev());
    NewTokens.insert(NewTokens.end(), PromptTokens.begin(), PromptTokens.end());
    NewTokens.insert(NewTokens.end(), Tokens.begin(), Tokens.end());
    Tokens = NewTokens;
  }

  return Tokens;
}

std::vector<int> DecodingTask::getSuppressTokens() {
  std::vector<int> SuppressTokens;

  if (Options.SuppressTokens) {
    if (std::holds_alternative<std::string>(*Options.SuppressTokens)) {
      std::string TokensString = std::get<std::string>(*Options.SuppressTokens);
      std::istringstream Iss(TokensString);
      std::string TokenString;
      while (std::getline(Iss, TokenString, ',')) {
        if (!TokenString.empty()) {
          SuppressTokens.push_back(std::stoi(TokenString));
        }
      }
    } else {
      SuppressTokens = std::get<std::vector<int>>(*Options.SuppressTokens);
    }
  }

  auto Iterator = std::find(SuppressTokens.begin(), SuppressTokens.end(), -1);
  if (Iterator != SuppressTokens.end()) {
    SuppressTokens.erase(std::remove_if(SuppressTokens.begin(),
                                        SuppressTokens.end(),
                                        [](int Token) { return Token < 0; }),
                         SuppressTokens.end());
    auto NonSpeechTokens = Tokenizer->getNonSpeechTokens();
    SuppressTokens.insert(SuppressTokens.end(), NonSpeechTokens.begin(),
                          NonSpeechTokens.end());
  } else if (!Options.SuppressTokens || SuppressTokens.empty()) {
    SuppressTokens.clear();
  } else {
    assumingUnreachable();
  }

  SuppressTokens.push_back(Tokenizer->getTranscribe());
  SuppressTokens.push_back(Tokenizer->getTranslate());
  SuppressTokens.push_back(Tokenizer->getSot());
  SuppressTokens.push_back(Tokenizer->getSotPrev());
  SuppressTokens.push_back(Tokenizer->getSotLm());

  SuppressTokens.push_back(Tokenizer->getNoSpeech());

  std::sort(SuppressTokens.begin(), SuppressTokens.end());
  SuppressTokens.erase(
      std::unique(SuppressTokens.begin(), SuppressTokens.end()),
      SuppressTokens.end());

  return SuppressTokens;
}

mx::array DecodingTask::getAudioFeatures(const mx::array &Mel) {
  bool Single = Mel.ndim() == 2;
  mx::array MelArray = Single ? mx::expand_dims(Mel, 0) : Mel;

  mx::array AudioFeatures = MelArray;

  // Skip encoder forward pass if already-encoded audio features were given
  if (AudioFeatures.shape(-2) != Model->Dims.NAudioCtx ||
      AudioFeatures.shape(-1) != Model->Dims.NAudioState) {
    AudioFeatures = Model->embedAudio(AudioFeatures);
  }

  return AudioFeatures;
}

std::pair<std::vector<std::string>,
          std::optional<std::vector<std::map<std::string, float>>>>
DecodingTask::detectLanguage(const mx::array &AudioFeatures,
                             mx::array &Tokens) {

  std::vector<std::string> Languages(AudioFeatures.shape(0),
                                     Options.Language.value_or("en"));
  std::optional<std::vector<std::map<std::string, float>>> LangProbs;

  if (!Options.Language || Options.Task == "lang_id") {
    // Call the global detectLanguage function
    auto [DetectedLanguageTokens, Probabilities] =
        whisper::detectLanguage(Model, AudioFeatures, Tokenizer);

    Languages.clear();
    for (const auto &ProbsMap : Probabilities) {
      auto MaxIterator = std::max_element(
          ProbsMap.begin(), ProbsMap.end(),
          [](const auto &A, const auto &B) { return A.second < B.second; });
      Languages.push_back(MaxIterator->first);
    }

    LangProbs = Probabilities;

    if (!Options.Language) {
      std::vector<int> Start = {0, SotIndex + 1};
      std::vector<int> End = {Tokens.shape()[0], SotIndex + 2};

      Tokens = slice_update(Tokens, DetectedLanguageTokens, Start, End);
    }
  }

  return {Languages, LangProbs};
}

std::tuple<mx::array, mx::array, mx::array>
DecodingTask::mainLoop(const mx::array &AudioFeatures,
                       const mx::array &Tokens) {

  int NBatch = Tokens.shape(0);
  mx::array CurrentTokens = Tokens;
  mx::array SumLogprobs = mx::zeros({NBatch}, mx::float32);
  bool Completed = false;

  auto StepFunction = [&](const mx::array &Inputs, const mx::array &AudioFeats,
                          const mx::array &TokSeq, const mx::array &SumLogp)
      -> std::tuple<mx::array, bool, mx::array, mx::array> {
    mx::array PreLogits = Inference->logits(Inputs, AudioFeats);
    mx::array Logits = take(PreLogits, PreLogits.shape(1) - 1, 1);
    for (const auto &Filter : LogitFilters) {
      Logits = Filter->apply(Logits, TokSeq);
    }
    auto [NextTokens, CompletedFlag, NextSumLogprobs] =
        Decoder->update(TokSeq, Logits, SumLogp);
    return std::make_tuple(NextTokens, CompletedFlag, NextSumLogprobs,
                           PreLogits);
  };

  auto [NextTokens, CompletedFlag, NextSumLogprobs, PreLogits] =
      StepFunction(CurrentTokens, AudioFeatures, CurrentTokens, SumLogprobs);

  CurrentTokens = NextTokens;
  SumLogprobs = NextSumLogprobs;
  Completed = CompletedFlag;

  mx::array NoSpeechProbs = mx::zeros({NBatch}, mx::float32);
  if (Tokenizer->getNoSpeech() != -1) {
    auto ProbsAtSot = mx::softmax(mx::take(PreLogits, SotIndex, 1), -1);
    NoSpeechProbs = mx::take(ProbsAtSot, Tokenizer->getNoSpeech(), 1);
  } else {
    NoSpeechProbs = mx::full({NBatch}, std::numeric_limits<float>::quiet_NaN(),
                             mx::float32);
  }

  mx::eval(CurrentTokens, SumLogprobs, NoSpeechProbs);
  for (int I = 1; I < SampleLen; ++I) {
    mx::array Inputs =
        take(CurrentTokens, mx::array({CurrentTokens.shape(1) - 1}), 1);

    if (CurrentTokens.shape(-1) > NCtx) {
      break;
    }
    auto [NextToks, NextCompleted, NextSumLogp, _] =
        StepFunction(Inputs, AudioFeatures, CurrentTokens, SumLogprobs);
    mx::eval(NextToks, NextSumLogp);

    if (Completed) {
      break;
    }

    CurrentTokens = NextToks;
    Completed = NextCompleted;
    SumLogprobs = NextSumLogp;
  }

  return std::make_tuple(CurrentTokens, SumLogprobs, NoSpeechProbs);
}

std::vector<DecodingResult> DecodingTask::run(const mx::array &Mel) {
  Inference->reset();
  Decoder->reset();
  int NAudio = Mel.shape(0);

  mx::array AudioFeatures = getAudioFeatures(Mel);

  mx::array Tokens =
      mx::array(InitialTokens.data(), {static_cast<int>(InitialTokens.size())},
                mx::int32);
  Tokens = mx::broadcast_to(Tokens,
                            {NAudio, static_cast<int>(InitialTokens.size())});
  auto [Languages, LangProbs] = detectLanguage(AudioFeatures, Tokens);

  if (Options.Task == "lang_id") {
    std::vector<DecodingResult> Results;
    for (int I = 0; I < NAudio; ++I) {
      DecodingResult Result;
      Result.AudioFeatures = mx::take(AudioFeatures, mx::array({I}), 0);
      Result.Language = Languages[I];
      if (LangProbs) {
        Result.LanguageProbs = (*LangProbs)[I];
      }
      Results.push_back(Result);
    }
    return Results;
  }

  if (NGroup > 1) {
    // tokens = tokens[:, None, :]
    Tokens = mx::expand_dims(Tokens, 1);

    // tokens = mx.broadcast_to(tokens, [n_audio, self.n_group,
    // len(self.initial_tokens)])
    std::vector<int> NewShape = {NAudio, NGroup,
                                 static_cast<int>(InitialTokens.size())};
    Tokens = mx::broadcast_to(Tokens, NewShape);

    // tokens = tokens.reshape(n_audio * self.n_group, len(self.initial_tokens))
    Tokens = mx::reshape(
        Tokens, {NAudio * NGroup, static_cast<int>(InitialTokens.size())});
  }
  // Call the main sampling loop
  auto [TokensResult, SumLogprobs, NoSpeechProbs] =
      mainLoop(AudioFeatures, Tokens);

  // Reshape the tensors to have (n_audio, n_group) as the first two dimensions
  AudioFeatures =
      mx::take(AudioFeatures, mx::arange(0, AudioFeatures.shape(0), NGroup), 0);
  NoSpeechProbs =
      mx::take(NoSpeechProbs, mx::arange(0, NoSpeechProbs.shape(0), NGroup), 0);

  // Ensure shapes are consistent
  if (AudioFeatures.shape(0) != NoSpeechProbs.shape(0) ||
      AudioFeatures.shape(0) != NAudio) {
    throw std::runtime_error(
        "Inconsistent audio features and no_speech_probs shapes");
  }

  TokensResult = mx::reshape(TokensResult, {NAudio, NGroup, -1});
  SumLogprobs = mx::reshape(SumLogprobs, {NAudio, NGroup});
  // Get the final candidates for each group, and slice between the first
  // sampled token and EOT
  auto [FinalizedTokens, FinalizedLogprobs] =
      Decoder->finalize(TokensResult, SumLogprobs);

  // tokens[..., self.sample_begin:]
  std::vector<int> SliceStart(FinalizedTokens.ndim(), 0);
  std::vector<int> SliceEnd = FinalizedTokens.shape();
  SliceStart[FinalizedTokens.ndim() - 1] = SampleBegin;
  FinalizedTokens = mx::slice(FinalizedTokens, SliceStart, SliceEnd);

  mx::eval(FinalizedTokens, FinalizedLogprobs, NoSpeechProbs);

  // Convert tokens to nested vectors and handle EOT
  std::vector<std::vector<std::vector<int>>> TokensList(NAudio);
  std::vector<std::vector<float>> SumLogprobsList(NAudio);

  for (int I = 0; I < NAudio; ++I) {
    TokensList[I].resize(NGroup);
    SumLogprobsList[I].resize(NGroup);

    for (int J = 0; J < NGroup; ++J) {
      // Extract tokens until EOT
      for (int K = 0; K < FinalizedTokens.shape(2); ++K) {
        int Dim1 = FinalizedTokens.shape(1) * FinalizedTokens.shape(2);
        int Dim2 = FinalizedTokens.shape(2);
        mx::array TokenArray =
            mx::take(FinalizedTokens, mx::array({I * Dim1 + J * Dim2 + K}));
        int Token = TokenArray.item<int>();
        if (Token == Tokenizer->getEot())
          break;
        TokensList[I][J].push_back(Token);
      }

      // Extract sum_logprobs
      mx::array LogprobArray = mx::take(
          FinalizedLogprobs, mx::array({I * FinalizedLogprobs.shape(1) + J}));
      SumLogprobsList[I][J] = LogprobArray.item<float>();
    }
  }

  std::vector<int> Selected = SequenceRanker->rank(TokensList, SumLogprobsList);

  // Extract final results
  std::vector<std::vector<int>> FinalTokens(NAudio);
  std::vector<std::string> Texts(NAudio);
  std::vector<float> FinalSumLogprobs(NAudio);
  std::vector<float> AvgLogprobs(NAudio);

  for (int I = 0; I < NAudio; ++I) {
    int SelectedIdx = Selected[I];
    FinalTokens[I] = TokensList[I][SelectedIdx];
    Texts[I] = Tokenizer->decode(FinalTokens[I]);

    Texts[I].erase(0, Texts[I].find_first_not_of(" \t\n\r\f\v"));
    Texts[I].erase(Texts[I].find_last_not_of(" \t\n\r\f\v") + 1);

    FinalSumLogprobs[I] = SumLogprobsList[I][SelectedIdx];
    AvgLogprobs[I] = FinalSumLogprobs[I] / (FinalTokens[I].size() + 1);
  }

  if (Texts.size() != Languages.size() ||
      Languages.size() != FinalTokens.size() ||
      FinalTokens.size() != AvgLogprobs.size() ||
      static_cast<int>(AvgLogprobs.size()) != NAudio) {
    throw std::runtime_error("inconsistent result lengths");
  }

  // Create final results
  std::vector<DecodingResult> Results;
  for (int I = 0; I < NAudio; ++I) {
    DecodingResult Result;
    Result.AudioFeatures = mx::take(AudioFeatures, I, 0);
    Result.Language = Languages[I];
    Result.Tokens = FinalTokens[I];
    Result.Text = Texts[I];
    Result.AvgLogprob = AvgLogprobs[I];

    mx::array NoSpeechArray = mx::take(NoSpeechProbs, mx::array({I}), 0);
    Result.NoSpeechProb = NoSpeechArray.item<float>();

    Result.Temperature = Options.Temperature;
    Result.CompressionRatio = compressionRatio(Result.Text);

    if (LangProbs) {
      Result.LanguageProbs = (*LangProbs)[I];
    }
    Results.push_back(Result);
  }

  return Results;
}

// Main decode function
std::variant<DecodingResult, std::vector<DecodingResult>>
decode(std::shared_ptr<Whisper> Model, const mx::array &Mel,
       const DecodingOptions &Options) {
  auto MelArray = Mel;
  if (Mel.ndim() == 2) {
    auto NewShape = Mel.shape();
    NewShape.insert(NewShape.begin(), 1);
    MelArray = reshape(Mel, NewShape);
  }
  auto Results = DecodingTask(Model, Options).run(MelArray);

  if (Results.size() == 1) {
    return Results[0];
  }
  return Results;
}

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX