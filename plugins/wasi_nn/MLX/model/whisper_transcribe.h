// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "whisper/decoding.h"
#include "whisper/tokenizer.h"
#include "whisper/whisper.h"
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace whisper {

// Audio processing constants
constexpr int DefaultSampleRate = 16000;
constexpr int DefaultNFft = 400;
constexpr int DefaultHopLength = 160;
constexpr int DefaultChunkLength = 30;
constexpr int DefaultNSamples =
    DefaultChunkLength * DefaultSampleRate; // 480000 samples
constexpr int DefaultNFrames =
    DefaultNSamples / DefaultHopLength; // 3000 frames
constexpr int DefaultFramesPerSecond = DefaultSampleRate / DefaultHopLength;
constexpr int DefaultNSamplesPerToken = DefaultHopLength * 2;

extern const std::vector<std::pair<std::string, std::string>> LANGUAGES;

// Word information for word-level timestamps
struct WordInfo {
  float Start;
  float End;
  std::string Word;
  float Probability;
};

// Segment information
struct TranscribeSegment {
  int Id;
  int Seek;
  float Start;
  float End;
  std::string Text;
  std::vector<int32_t> Tokens;
  float Temperature;
  float AvgLogprob;
  float CompressionRatio;
  float NoSpeechProb;
  std::vector<WordInfo> Words;
};

// Complete transcribe result
struct TranscribeResult {
  std::string Text;
  std::vector<TranscribeSegment> Segments;
  std::string Language;
};

// Audio processing functions
mx::array loadAudio(const std::string &FilePath,
                    int SampleRate = DefaultSampleRate);
mx::array padOrTrim(const mx::array &Array, int Length = DefaultNSamples,
                    int Axis = -1);
mx::array logMelSpectrogram(const mx::array &Audio, int NMels = 80,
                            int Padding = 0);

// Utility functions
std::string formatTimestamp(float Seconds);
std::optional<float> getEnd(const std::vector<TranscribeSegment> &Segments);

// Decoding functions
DecodingResult
decodeWithFallback(std::shared_ptr<whisper::Whisper> Model,
                   const mx::array &MelSegment,
                   const DecodingOptions &DecodeOptions,
                   const std::vector<float> &Temperatures,
                   std::unique_ptr<whisper::Tokenizer> &Tokenizer,
                   std::optional<float> CompressionRatioThreshold,
                   std::optional<float> LogprobThreshold,
                   std::optional<float> NoSpeechThreshold);

// Word-level timestamp functions
void addWordTimestamps(std::vector<TranscribeSegment> &Segments,
                       std::shared_ptr<whisper::Whisper> Model,
                       std::unique_ptr<whisper::Tokenizer> &Tokenizer,
                       const mx::array &Mel, int NumFrames,
                       const std::string &PrependPunctuations,
                       const std::string &AppendPunctuations,
                       float LastSpeechTimestamp = 0.0f);

// Anomaly detection functions
float wordAnomalyScore(const WordInfo &Word);
bool isSegmentAnomaly(const std::optional<TranscribeSegment> &Segment);
std::optional<TranscribeSegment>
nextWordsSegment(const std::vector<TranscribeSegment> &Segments);

// Main transcribe function
TranscribeResult
transcribe(const std::variant<std::string, mx::array> &Audio,
           std::shared_ptr<whisper::Whisper> Model,
           std::optional<bool> Verbose = std::nullopt,
           std::variant<float, std::vector<float>> Temperature =
               std::vector<float>{0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f},
           std::optional<float> CompressionRatioThreshold = 2.4f,
           std::optional<float> LogprobThreshold = -1.0f,
           std::optional<float> NoSpeechThreshold = 0.6f,
           bool ConditionOnPreviousText = true,
           std::optional<std::string> InitialPrompt = std::nullopt,
           bool WordTimestamps = false,
           const std::string &PrependPunctuations =
               "\"'“¿([{-\"'.。,，!！?？:：”)]}、",
           const std::string &AppendPunctuations = "\"'.,!?:\")]},",
           std::variant<std::string, std::vector<float>> ClipTimestamps = "0",
           std::optional<float> HallucinationSilenceThreshold = std::nullopt,
           const DecodingOptions &DecodeOptions = DecodingOptions());

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX