// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "tokenizer.h"
#include "mlx/base.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace WasmEdge::Host::WASINN::MLX {
namespace whisper {

const std::vector<std::pair<std::string, std::string>> LANGUAGES = {
    {"en", "english"},     {"zh", "chinese"},    {"de", "german"},
    {"es", "spanish"},     {"ru", "russian"},    {"ko", "korean"},
    {"fr", "french"},      {"ja", "japanese"},   {"pt", "portuguese"},
    {"tr", "turkish"},     {"pl", "polish"},     {"ca", "catalan"},
    {"nl", "dutch"},       {"ar", "arabic"},     {"sv", "swedish"},
    {"it", "italian"},     {"id", "indonesian"}, {"hi", "hindi"},
    {"fi", "finnish"},     {"vi", "vietnamese"}, {"he", "hebrew"},
    {"uk", "ukrainian"},   {"el", "greek"},      {"ms", "malay"},
    {"cs", "czech"},       {"ro", "romanian"},   {"da", "danish"},
    {"hu", "hungarian"},   {"ta", "tamil"},      {"no", "norwegian"},
    {"th", "thai"},        {"ur", "urdu"},       {"hr", "croatian"},
    {"bg", "bulgarian"},   {"lt", "lithuanian"}, {"la", "latin"},
    {"mi", "maori"},       {"ml", "malayalam"},  {"cy", "welsh"},
    {"sk", "slovak"},      {"te", "telugu"},     {"fa", "persian"},
    {"lv", "latvian"},     {"bn", "bengali"},    {"sr", "serbian"},
    {"az", "azerbaijani"}, {"sl", "slovenian"},  {"kn", "kannada"},
    {"et", "estonian"},    {"mk", "macedonian"}, {"br", "breton"},
    {"eu", "basque"},      {"is", "icelandic"},  {"hy", "armenian"},
    {"ne", "nepali"},      {"mn", "mongolian"},  {"bs", "bosnian"},
    {"kk", "kazakh"},      {"sq", "albanian"},   {"sw", "swahili"},
    {"gl", "galician"},    {"mr", "marathi"},    {"pa", "punjabi"},
    {"si", "sinhala"},     {"km", "khmer"},      {"sn", "shona"},
    {"yo", "yoruba"},      {"so", "somali"},     {"af", "afrikaans"},
    {"oc", "occitan"},     {"ka", "georgian"},   {"be", "belarusian"},
    {"tg", "tajik"},       {"sd", "sindhi"},     {"gu", "gujarati"},
    {"am", "amharic"},     {"yi", "yiddish"},    {"lo", "lao"},
    {"uz", "uzbek"},       {"fo", "faroese"},    {"ht", "haitian creole"},
    {"ps", "pashto"},      {"tk", "turkmen"},    {"nn", "nynorsk"},
    {"mt", "maltese"},     {"sa", "sanskrit"},   {"lb", "luxembourgish"},
    {"my", "myanmar"},     {"bo", "tibetan"},    {"tl", "tagalog"},
    {"mg", "malagasy"},    {"as", "assamese"},   {"tt", "tatar"},
    {"haw", "hawaiian"},   {"ln", "lingala"},    {"ha", "hausa"},
    {"ba", "bashkir"},     {"jw", "javanese"},   {"su", "sundanese"},
    {"yue", "cantonese"}};

// Helper function to find language by code
std::string findLanguageByCode(const std::string &Code) {
  for (const auto &[LangCode, Language] : LANGUAGES) {
    if (LangCode == Code) {
      return Language;
    }
  }
  return "";
}

// Helper function to check if language code exists
bool languageCodeExists(const std::string &Code) {
  for (const auto &[LangCode, Language] : LANGUAGES) {
    if (LangCode == Code) {
      return true;
    }
  }
  return false;
}

const std::unordered_map<std::string, std::string> ToLanguageCode = []() {
  std::unordered_map<std::string, std::string> ToLanguageCodeMap;

  // Add language to code mappings
  for (const auto &[Code, Language] : LANGUAGES) {
    ToLanguageCodeMap[Language] = Code;
  }

  // Add aliases
  ToLanguageCodeMap["burmese"] = "my";
  ToLanguageCodeMap["valencian"] = "ca";
  ToLanguageCodeMap["flemish"] = "nl";
  ToLanguageCodeMap["haitian"] = "ht";
  ToLanguageCodeMap["letzeburgesch"] = "lb";
  ToLanguageCodeMap["pushto"] = "ps";
  ToLanguageCodeMap["panjabi"] = "pa";
  ToLanguageCodeMap["moldavian"] = "ro";
  ToLanguageCodeMap["moldovan"] = "ro";
  ToLanguageCodeMap["sinhalese"] = "si";
  ToLanguageCodeMap["castilian"] = "es";
  ToLanguageCodeMap["mandarin"] = "zh";

  return ToLanguageCodeMap;
}();

namespace {
// Modern base64 decode implementation
// Based on RFC 4648 standard with better error handling
std::string base64Decode(const std::string &Encoded) {
  // Standard base64 alphabet lookup table
  static const std::array<int, 256> DecodeTable = []() {
    std::array<int, 256> Table{};
    // Initialize all values to -1 (invalid)
    std::fill(Table.begin(), Table.end(), -1);

    // Set valid characters
    for (int I = 0; I < 26; ++I) {
      Table['A' + I] = I;      // A-Z: 0-25
      Table['a' + I] = I + 26; // a-z: 26-51
    }
    for (int I = 0; I < 10; ++I) {
      Table['0' + I] = I + 52; // 0-9: 52-61
    }
    Table['+'] = 62;
    Table['/'] = 63;
    // Padding character
    Table['='] = -2;

    return Table;
  }();

  if (Encoded.empty() || Encoded == "=") {
    return {};
  }

  // Remove whitespace and validate length
  std::string Cleaned;
  Cleaned.reserve(Encoded.size());
  for (char C : Encoded) {
    if (C != ' ' && C != '\t' && C != '\r' && C != '\n') {
      Cleaned.push_back(C);
    }
  }

  if (Cleaned.size() % 4 != 0) {
    throw std::invalid_argument("Invalid base64 string length");
  }

  std::string Result;
  Result.reserve((Cleaned.size() * 3) / 4);

  for (size_t I = 0; I < Cleaned.size(); I += 4) {
    std::array<int, 4> Values{};
    int PaddingCount = 0;

    // Decode 4 characters at a time
    for (int J = 0; J < 4; ++J) {
      unsigned char C = static_cast<unsigned char>(Cleaned[I + J]);
      int Val = DecodeTable[C];

      if (Val == -1) {
        throw std::invalid_argument("Invalid base64 character: " +
                                    std::to_string(C));
      }
      if (Val == -2) { // Padding
        Values[J] = 0;
        ++PaddingCount;
      } else {
        if (PaddingCount > 0) {
          throw std::invalid_argument("Invalid padding in base64 string");
        }
        Values[J] = Val;
      }
    }

    // Convert 4 base64 chars to 3 bytes
    int Combined =
        (Values[0] << 18) | (Values[1] << 12) | (Values[2] << 6) | Values[3];

    // Extract bytes based on padding
    if (PaddingCount == 0) {
      Result.push_back(static_cast<char>((Combined >> 16) & 0xFF));
      Result.push_back(static_cast<char>((Combined >> 8) & 0xFF));
      Result.push_back(static_cast<char>(Combined & 0xFF));
    } else if (PaddingCount == 1) {
      Result.push_back(static_cast<char>((Combined >> 16) & 0xFF));
      Result.push_back(static_cast<char>((Combined >> 8) & 0xFF));
    } else if (PaddingCount == 2) {
      Result.push_back(static_cast<char>((Combined >> 16) & 0xFF));
    } else {
      throw std::invalid_argument("Invalid padding count in base64 string");
    }
  }

  return Result;
}
} // namespace

// Encoding implementation
Encoding::Encoding(const std::string &Name, int ExplicitNVocab,
                   const std::string &PatStr,
                   const std::unordered_map<std::string, int> &MergeableRanks,
                   const std::unordered_map<std::string, int> &SpecialTokens)
    : Name(Name), PatStr(PatStr), MergeableRanks(MergeableRanks),
      SpecialTokens(SpecialTokens) {

  EotToken = 50257; // Default EOT token

  // Build reverse mapping
  for (const auto &[Token, Id] : MergeableRanks) {
    TokenToString[Id] = Token;
  }
  for (const auto &[Token, Id] : SpecialTokens) {
    TokenToString[Id] = Token;
    SpecialTokensSet.insert(Token);
  }
}

std::vector<int> Encoding::encode(const std::string &Text) const {
  std::vector<int> Result;

  if (Text.empty()) {
    return Result;
  }

  // Handle the simplified cases for symbols that are commonly in vocab
  // First try to encode the text as a single token
  auto DirectIt = MergeableRanks.find(Text);
  if (DirectIt != MergeableRanks.end()) {
    Result.push_back(DirectIt->second);
    return Result;
  }

  // For complex text, we need a better approach than just splitting by spaces
  // This is a more sophisticated approach that handles individual characters
  // and symbols

  size_t I = 0;
  while (I < Text.length()) {
    // Try to find the longest matching token starting at position I
    std::string LongestMatch;
    int LongestMatchToken = -1;

    // Look for longest match from current position
    for (size_t Len = std::min(Text.length() - I, size_t(10)); Len > 0; --Len) {
      std::string Candidate = Text.substr(I, Len);
      auto It = MergeableRanks.find(Candidate);
      if (It != MergeableRanks.end()) {
        LongestMatch = Candidate;
        LongestMatchToken = It->second;
        break; // Take the first (longest) match
      }
    }

    if (LongestMatchToken != -1) {
      Result.push_back(LongestMatchToken);
      I += LongestMatch.length();
    } else {
      // If no match found, try single character
      std::string SingleChar = Text.substr(I, 1);
      auto CharIt = MergeableRanks.find(SingleChar);
      if (CharIt != MergeableRanks.end()) {
        Result.push_back(CharIt->second);
      } else {
        // Handle UTF-8 sequences - try to find multi-byte character
        size_t CharLen = 1;
        unsigned char FirstByte = static_cast<unsigned char>(Text[I]);
        if ((FirstByte & 0x80) != 0) {
          // UTF-8 multi-byte character
          if ((FirstByte & 0xE0) == 0xC0)
            CharLen = 2;
          else if ((FirstByte & 0xF0) == 0xE0)
            CharLen = 3;
          else if ((FirstByte & 0xF8) == 0xF0)
            CharLen = 4;
        }

        if (I + CharLen <= Text.length()) {
          std::string MultiByteChar = Text.substr(I, CharLen);
          auto MultiIt = MergeableRanks.find(MultiByteChar);
          if (MultiIt != MergeableRanks.end()) {
            Result.push_back(MultiIt->second);
            I += CharLen;
            continue;
          }
        }

        // If we still can't find it, encode as bytes
        for (size_t J = 0; J < CharLen && I + J < Text.length(); ++J) {
          unsigned char Byte = static_cast<unsigned char>(Text[I + J]);
          // Try to find byte encoding - tiktoken often has byte-level tokens
          std::string ByteStr(1, static_cast<char>(Byte));
          auto ByteIt = MergeableRanks.find(ByteStr);
          if (ByteIt != MergeableRanks.end()) {
            Result.push_back(ByteIt->second);
          }
        }
        I += CharLen;
      }
    }
  }

  return Result;
}

std::string Encoding::decode(const std::vector<int> &TokenIds) const {
  std::string Result;
  for (int Id : TokenIds) {
    auto It = TokenToString.find(Id);
    if (It != TokenToString.end()) {
      Result += It->second;
    }
  }
  return Result;
}

int Encoding::encodeSingleToken(const std::string &Token) const {
  auto It = SpecialTokens.find(Token);
  if (It != SpecialTokens.end()) {
    return It->second;
  }

  auto MergeIt = MergeableRanks.find(Token);
  if (MergeIt != MergeableRanks.end()) {
    return MergeIt->second;
  }

  throw std::runtime_error("Token not found: " + Token);
}
// Tokenizer implementation
Tokenizer::Tokenizer(std::unique_ptr<Encoding> EncodingPtr, int NumLanguages,
                     const std::optional<std::string> &Language,
                     const std::optional<std::string> &Task)
    : EncodingPtr(std::move(EncodingPtr)), NumLanguages(NumLanguages),
      Language(Language), Task(Task) {

  for (const auto &Special : this->EncodingPtr->SpecialTokensSet) {
    int SpecialToken = this->EncodingPtr->encodeSingleToken(Special);
    SpecialTokens[Special] = SpecialToken;
  }

  // Build SOT sequence
  int Sot = SpecialTokens["<|startoftranscript|>"];
  int Translate = SpecialTokens["<|translate|>"];
  int Transcribe = SpecialTokens["<|transcribe|>"];
  std::vector<std::string> Langs;
  for (const auto &[Code, Name] : LANGUAGES) {
    Langs.push_back(Code);
    if (static_cast<int>(Langs.size()) >= NumLanguages)
      break;
  }

  SotSequence = {Sot};
  if (Language.has_value()) {
    auto LangIt = std::find(Langs.begin(), Langs.end(), Language.value());
    if (LangIt != Langs.end()) {
      int LangIndex = std::distance(Langs.begin(), LangIt);
      SotSequence.push_back(Sot + 1 + LangIndex);
    }
  }
  if (Task.has_value()) {
    int TaskToken = (Task.value() == "transcribe") ? Transcribe : Translate;
    SotSequence.push_back(TaskToken);
  }
}

std::vector<int> Tokenizer::encode(const std::string &Text) const {
  return EncodingPtr->encode(Text);
}

std::string Tokenizer::decode(const std::vector<int> &TokenIds) const {
  std::vector<int> FilteredTokens;
  int TimestampBegin = getTimestampBegin();

  for (int Token : TokenIds) {
    if (Token < TimestampBegin) {
      FilteredTokens.push_back(Token);
    }
  }

  return EncodingPtr->decode(FilteredTokens);
}

std::string
Tokenizer::decodeWithTimestamps(const std::vector<int> &TokenIds) const {
  return EncodingPtr->decode(TokenIds);
}

// Cached property implementations
int Tokenizer::getEot() const {
  if (!CachedEot.has_value()) {
    CachedEot = EncodingPtr->EotToken;
  }
  return CachedEot.value();
}

int Tokenizer::getTranscribe() const {
  if (!CachedTranscribe.has_value()) {
    CachedTranscribe = SpecialTokens.at("<|transcribe|>");
  }
  return CachedTranscribe.value();
}

int Tokenizer::getTranslate() const {
  if (!CachedTranslate.has_value()) {
    CachedTranslate = SpecialTokens.at("<|translate|>");
  }
  return CachedTranslate.value();
}

int Tokenizer::getSot() const {
  if (!CachedSot.has_value()) {
    CachedSot = SpecialTokens.at("<|startoftranscript|>");
  }
  return CachedSot.value();
}

int Tokenizer::getSotLm() const {
  if (!CachedSotLm.has_value()) {
    CachedSotLm = SpecialTokens.at("<|startoflm|>");
  }
  return CachedSotLm.value();
}

int Tokenizer::getSotPrev() const {
  if (!CachedSotPrev.has_value()) {
    CachedSotPrev = SpecialTokens.at("<|startofprev|>");
  }
  return CachedSotPrev.value();
}

int Tokenizer::getNoSpeech() const {
  if (!CachedNoSpeech.has_value()) {
    CachedNoSpeech = SpecialTokens.at("<|nospeech|>");
  }
  return CachedNoSpeech.value();
}

int Tokenizer::getNoTimestamps() const {
  if (!CachedNoTimestamps.has_value()) {
    CachedNoTimestamps = SpecialTokens.at("<|notimestamps|>");
  }
  return CachedNoTimestamps.value();
}

int Tokenizer::getTimestampBegin() const {
  if (!CachedTimestampBegin.has_value()) {
    CachedTimestampBegin = SpecialTokens.at("<|0.00|>");
  }
  return CachedTimestampBegin.value();
}

int Tokenizer::languageToken() const {
  if (!Language.has_value()) {
    throw std::runtime_error(
        "This tokenizer does not have language token configured");
  }
  return toLanguageToken(Language.value());
}

int Tokenizer::toLanguageToken(const std::string &Language) const {
  std::string TokenName = "<|" + Language + "|>";
  auto It = SpecialTokens.find(TokenName);
  if (It != SpecialTokens.end()) {
    return It->second;
  }
  throw std::runtime_error("Language " + Language + " not found in tokenizer.");
}

std::vector<int> Tokenizer::getAllLanguageTokens() const {
  if (!CachedAllLanguageTokens.has_value()) {
    std::vector<int> Result;

    std::vector<std::string> SortedLanguageTokens;
    for (const auto &[Token, TokenId] : SpecialTokens) {
      std::string Stripped = Token;
      const std::string CharsToStrip = "<|>";
      while (!Stripped.empty() &&
             CharsToStrip.find(Stripped.front()) != std::string::npos) {
        Stripped.erase(0, 1);
      }
      while (!Stripped.empty() &&
             CharsToStrip.find(Stripped.back()) != std::string::npos) {
        Stripped.pop_back();
      }
      if (languageCodeExists(Stripped)) {
        SortedLanguageTokens.push_back(Token);
      }
    }

    std::sort(SortedLanguageTokens.begin(), SortedLanguageTokens.end());

    for (const std::string &Token : SortedLanguageTokens) {
      auto It = SpecialTokens.find(Token);
      if (It != SpecialTokens.end()) {
        Result.push_back(It->second);
      }
    }
    if (static_cast<int>(Result.size()) > NumLanguages) {
      Result.resize(NumLanguages);
    }

    CachedAllLanguageTokens = Result;
  }
  return CachedAllLanguageTokens.value();
}

std::vector<std::string> Tokenizer::getAllLanguageCodes() const {
  if (!CachedAllLanguageCodes.has_value()) {
    std::vector<std::string> Result;
    auto LanguageTokens = getAllLanguageTokens();

    for (int Token : LanguageTokens) {
      std::string Decoded = EncodingPtr->decode({Token});
      // Remove <| and |> from decoded token
      if (Decoded.size() > 4 && Decoded.substr(0, 2) == "<|" &&
          Decoded.substr(Decoded.size() - 2) == "|>") {
        Result.push_back(Decoded.substr(2, Decoded.size() - 4));
      }
    }

    CachedAllLanguageCodes = Result;
  }
  return CachedAllLanguageCodes.value();
}

std::vector<int> Tokenizer::getSotSequenceIncludingNotimestamps() const {
  if (!CachedSotSequenceIncludingNotimestamps.has_value()) {
    std::vector<int> Result = SotSequence;
    Result.push_back(getNoTimestamps());
    CachedSotSequenceIncludingNotimestamps = Result;
  }
  return CachedSotSequenceIncludingNotimestamps.value();
}

std::vector<int> Tokenizer::getNonSpeechTokens() const {
  if (!CachedNonSpeechTokens.has_value()) {
    std::unordered_set<int> ResultSet;

    std::vector<std::string> Symbols = {"\"", "#", "(",  ")",  "*",  "+", "/",
                                        ":",  ";", "<",  "=",  ">",  "@", "[",
                                        "\\", "]", "^",  "_",  "`",  "{", "|",
                                        "}",  "~", "「", "」", "『", "』"};

    std::vector<std::string> AdditionalSymbols = {
        "<<", ">>", "<<<", ">>>", "--", "---", "-(", "-[", "('", "(\"",
        "((", "))", "(((", ")))", "[[", "]]",  "{{", "}}", "♪♪", "♪♪♪"};
    Symbols.insert(Symbols.end(), AdditionalSymbols.begin(),
                   AdditionalSymbols.end());

    std::vector<std::string> Miscellaneous = {"♩", "♪", "♫", "♬",
                                              "♭", "♮", "♯"};

    auto DashTokens = EncodingPtr->encode(" -");
    if (!DashTokens.empty()) {
      ResultSet.insert(DashTokens[0]);
    }

    auto QuoteTokens = EncodingPtr->encode(" '");
    if (!QuoteTokens.empty()) {
      ResultSet.insert(QuoteTokens[0]);
    }

    // Process all symbols
    std::vector<std::string> AllSymbols = Symbols;
    AllSymbols.insert(AllSymbols.end(), Miscellaneous.begin(),
                      Miscellaneous.end());

    for (const std::string &Symbol : AllSymbols) {
      try {
        // Try encoding the symbol directly
        auto DirectTokens = EncodingPtr->encode(Symbol);
        bool IsMiscellaneous =
            std::find(Miscellaneous.begin(), Miscellaneous.end(), Symbol) !=
            Miscellaneous.end();

        if (DirectTokens.size() == 1 || IsMiscellaneous) {
          if (!DirectTokens.empty()) {
            ResultSet.insert(DirectTokens[0]);
          }
        }

        // Try encoding the symbol with a space prefix
        auto SpacedTokens = EncodingPtr->encode(" " + Symbol);
        if (SpacedTokens.size() == 1 || IsMiscellaneous) {
          if (!SpacedTokens.empty()) {
            ResultSet.insert(SpacedTokens[0]);
          }
        }
      } catch (...) {
        // Ignore encoding errors for individual symbols
      }
    }

    // Convert set to sorted vector
    std::vector<int> Result(ResultSet.begin(), ResultSet.end());
    std::sort(Result.begin(), Result.end());

    CachedNonSpeechTokens = Result;
  }
  return CachedNonSpeechTokens.value();
}

std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
Tokenizer::splitToWordTokens(const std::vector<int> &Tokens) const {
  std::unordered_set<std::string> NoSpaceLanguages = {"zh", "ja", "th",
                                                      "lo", "my", "yue"};

  if (Language.has_value() && NoSpaceLanguages.count(Language.value())) {
    return splitTokensOnUnicode(Tokens);
  }

  return splitTokensOnSpaces(Tokens);
}

std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
Tokenizer::splitTokensOnUnicode(const std::vector<int> &Tokens) const {
  std::string DecodedFull = decodeWithTimestamps(Tokens);
  const std::string ReplacementChar = "\uFFFD";

  std::vector<std::string> Words;
  std::vector<std::vector<int>> WordTokens;
  std::vector<int> CurrentTokens;
  size_t UnicodeOffset = 0;

  for (int Token : Tokens) {
    CurrentTokens.push_back(Token);
    std::string Decoded = decodeWithTimestamps(CurrentTokens);

    bool ValidUnicode =
        (Decoded.find(ReplacementChar) == std::string::npos) ||
        (UnicodeOffset + Decoded.find(ReplacementChar) < DecodedFull.size() &&
         DecodedFull.substr(UnicodeOffset + Decoded.find(ReplacementChar),
                            ReplacementChar.length()) == ReplacementChar);

    if (ValidUnicode) {
      Words.push_back(Decoded);
      WordTokens.push_back(CurrentTokens);
      CurrentTokens.clear();
      UnicodeOffset += Decoded.size();
    }
  }

  return {Words, WordTokens};
}

std::pair<std::vector<std::string>, std::vector<std::vector<int>>>
Tokenizer::splitTokensOnSpaces(const std::vector<int> &Tokens) const {
  auto [Subwords, SubwordTokensList] = splitTokensOnUnicode(Tokens);

  std::vector<std::string> Words;
  std::vector<std::vector<int>> WordTokens;

  for (size_t I = 0; I < Subwords.size(); ++I) {
    const std::string &Subword = Subwords[I];
    const std::vector<int> &SubwordTokens = SubwordTokensList[I];

    bool Special = !SubwordTokens.empty() && SubwordTokens[0] >= getEot();
    bool WithSpace = !Subword.empty() && Subword[0] == ' ';

    // Check if it's punctuation
    std::string Trimmed = Subword;
    Trimmed.erase(0, Trimmed.find_first_not_of(" \t\n\r"));
    Trimmed.erase(Trimmed.find_last_not_of(" \t\n\r") + 1);
    bool Punctuation = Trimmed.size() == 1 && std::ispunct(Trimmed[0]);

    if (Special || WithSpace || Punctuation || Words.empty()) {
      Words.push_back(Subword);
      WordTokens.push_back(SubwordTokens);
    } else {
      Words.back() += Subword;
      WordTokens.back().insert(WordTokens.back().end(), SubwordTokens.begin(),
                               SubwordTokens.end());
    }
  }

  return {Words, WordTokens};
}

// Factory functions
std::unique_ptr<Encoding> getEncoding(const std::string &Name,
                                      int NumLanguages) {
  std::string VocabPath = "assets/" + Name + ".tiktoken";

  std::unordered_map<std::string, int> Ranks;
  std::ifstream File(VocabPath);

  if (!File.is_open()) {
    throw std::runtime_error("Failed to open vocab file: " + VocabPath);
  }

  std::string Line;
  while (std::getline(File, Line)) {
    if (Line.empty())
      continue;

    std::istringstream Iss(Line);
    std::string Token, RankStr;
    if (Iss >> Token >> RankStr) {
      std::string DecodedToken = base64Decode(Token);
      Ranks[DecodedToken] = std::stoi(RankStr);
    }
  }
  File.close();

  int NVocab = Ranks.size();
  std::unordered_map<std::string, int> SpecialTokens;

  // Build special tokens
  std::vector<std::string> Specials = {"<|endoftext|>",
                                       "<|startoftranscript|>"};

  for (const auto &[Code, Name] : LANGUAGES) {
    Specials.push_back("<|" + Code + "|>");
    if (static_cast<int>(Specials.size()) >= NumLanguages + 2)
      break;
  }

  Specials.insert(Specials.end(),
                  {"<|translate|>", "<|transcribe|>", "<|startoflm|>",
                   "<|startofprev|>", "<|nospeech|>", "<|notimestamps|>"});

  // Add timestamp tokens
  for (int I = 0; I < 1501; ++I) {
    std::ostringstream Oss;
    Oss << "<|" << std::fixed << std::setprecision(2) << (I * 0.02) << "|>";
    Specials.push_back(Oss.str());
  }

  for (const std::string &Token : Specials) {
    SpecialTokens[Token] = NVocab++;
  }

  std::string PatStr =
      R"('s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+)";

  return std::make_unique<Encoding>(VocabPath, NVocab, PatStr, Ranks,
                                    SpecialTokens);
}

std::unique_ptr<Tokenizer>
getTokenizer(bool Multilingual, int NumLanguages,
             const std::optional<std::string> &Language,
             const std::optional<std::string> &Task) {
  std::optional<std::string> ProcessedLanguage = Language;

  if (ProcessedLanguage.has_value()) {
    std::string Lang = ProcessedLanguage.value();
    std::transform(Lang.begin(), Lang.end(), Lang.begin(), ::tolower);

    if (!languageCodeExists(Lang)) {
      auto It = ToLanguageCode.find(Lang);
      if (It != ToLanguageCode.end()) {
        ProcessedLanguage = It->second;
      } else {
        throw std::runtime_error("Unsupported language: " + Lang);
      }
    } else {
      ProcessedLanguage = Lang;
    }
  }

  std::string EncodingName;
  std::optional<std::string> FinalLanguage;
  std::optional<std::string> FinalTask;

  if (Multilingual) {
    EncodingName = "multilingual";
    FinalLanguage = ProcessedLanguage.value_or("en");
    FinalTask = Task.value_or("transcribe");
  } else {
    EncodingName = "gpt2";
    FinalLanguage = std::nullopt;
    FinalTask = std::nullopt;
  }

  auto Encoding = getEncoding(EncodingName, NumLanguages);

  return std::make_unique<Tokenizer>(std::move(Encoding), NumLanguages,
                                     FinalLanguage, FinalTask);
}

std::unique_ptr<Tokenizer>
createWhisperTokenizer(const std::optional<std::string> &Language,
                       const std::optional<std::string> &Task) {
  return getTokenizer(true, 99, Language, Task);
}

} // namespace whisper
} // namespace WasmEdge::Host::WASINN::MLX