// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace whisper {

extern const std::vector<std::pair<std::string, std::string>> LANGUAGES;
extern const std::unordered_map<std::string, std::string> ToLanguageCode;

std::string findLanguageByCode(const std::string &Code);
bool languageCodeExists(const std::string &Code);

class Encoding {
public:
  Encoding(const std::string &Name, int ExplicitNVocab,
           const std::string &PatStr,
           const std::unordered_map<std::string, int> &MergeableRanks,
           const std::unordered_map<std::string, int> &SpecialTokens);

  std::vector<int> encode(const std::string &Text) const;
  std::string decode(const std::vector<int> &TokenIds) const;
  int encodeSingleToken(const std::string &Token) const;

  int EotToken;
  std::unordered_set<std::string> SpecialTokensSet;

private:
  std::string Name;
  std::string PatStr;
  std::unordered_map<std::string, int> MergeableRanks;
  std::unordered_map<std::string, int> SpecialTokens;
  std::unordered_map<int, std::string> TokenToString;
};

class Tokenizer {
public:
  Tokenizer(std::unique_ptr<Encoding> EncodingPtr, int NumLanguages,
            const std::optional<std::string> &Language = std::nullopt,
            const std::optional<std::string> &Task = std::nullopt);

  std::vector<int> encode(const std::string &Text) const;
  std::string decode(const std::vector<int> &TokenIds) const;
  std::string decodeWithTimestamps(const std::vector<int> &TokenIds) const;

  // Cached properties
  int getEot() const;
  int getTranscribe() const;
  int getTranslate() const;
  int getSot() const;
  int getSotLm() const;
  int getSotPrev() const;
  int getNoSpeech() const;
  int getNoTimestamps() const;
  int getTimestampBegin() const;
  int languageToken() const;
  int toLanguageToken(const std::string &Language) const;
  std::vector<int> getAllLanguageTokens() const;
  std::vector<std::string> getAllLanguageCodes() const;
  std::vector<int> getSotSequenceIncludingNotimestamps() const;
  std::vector<int> getNonSpeechTokens() const;

  std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
  splitToWordTokens(const std::vector<int> &Tokens) const;

  // Public members
  std::unique_ptr<Encoding> EncodingPtr;
  int NumLanguages;
  std::optional<std::string> Language;
  std::optional<std::string> Task;
  std::vector<int> SotSequence;
  std::unordered_map<std::string, int> SpecialTokens;

private:
  std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
  splitTokensOnUnicode(const std::vector<int> &Tokens) const;

  std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
  splitTokensOnSpaces(const std::vector<int> &Tokens) const;

  // Cached values
  mutable std::optional<int> CachedEot;
  mutable std::optional<int> CachedTranscribe;
  mutable std::optional<int> CachedTranslate;
  mutable std::optional<int> CachedSot;
  mutable std::optional<int> CachedSotLm;
  mutable std::optional<int> CachedSotPrev;
  mutable std::optional<int> CachedNoSpeech;
  mutable std::optional<int> CachedNoTimestamps;
  mutable std::optional<int> CachedTimestampBegin;
  mutable std::optional<std::vector<int>> CachedAllLanguageTokens;
  mutable std::optional<std::vector<std::string>> CachedAllLanguageCodes;
  mutable std::optional<std::vector<int>>
      CachedSotSequenceIncludingNotimestamps;
  mutable std::optional<std::vector<int>> CachedNonSpeechTokens;
};

std::unique_ptr<Encoding> getEncoding(const std::string &Name = "gpt2",
                                      int NumLanguages = 99);

std::unique_ptr<Tokenizer>
getTokenizer(bool Multilingual, int NumLanguages = 99,
             const std::optional<std::string> &Language = std::nullopt,
             const std::optional<std::string> &Task = std::nullopt);

// Helper function to create a tokenizer for whisper transcription
std::unique_ptr<Tokenizer> createWhisperTokenizer(
    const std::optional<std::string> &Language = std::nullopt,
    const std::optional<std::string> &Task = std::nullopt);

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX
