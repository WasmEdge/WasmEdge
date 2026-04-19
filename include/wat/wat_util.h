// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/types.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace WAT {

/// Parse a hex digit to its value (0-15); -1 if invalid.
inline int hexDigit(char C) {
  if (C >= '0' && C <= '9')
    return C - '0';
  if (C >= 'a' && C <= 'f')
    return C - 'a' + 10;
  if (C >= 'A' && C <= 'F')
    return C - 'A' + 10;
  return -1;
}

/// Encode a code point as UTF-8 into a container. Code points above U+10FFFF
/// are silently ignored.
template <typename Container>
inline void encodeUTF8(uint32_t CP, Container &Out) {
  using V = typename Container::value_type;
  if (CP <= 0x7F) {
    Out.push_back(static_cast<V>(CP));
  } else if (CP <= 0x7FF) {
    Out.push_back(static_cast<V>(0xC0 | (CP >> 6)));
    Out.push_back(static_cast<V>(0x80 | (CP & 0x3F)));
  } else if (CP <= 0xFFFF) {
    Out.push_back(static_cast<V>(0xE0 | (CP >> 12)));
    Out.push_back(static_cast<V>(0x80 | ((CP >> 6) & 0x3F)));
    Out.push_back(static_cast<V>(0x80 | (CP & 0x3F)));
  } else if (CP <= 0x10FFFF) {
    Out.push_back(static_cast<V>(0xF0 | (CP >> 18)));
    Out.push_back(static_cast<V>(0x80 | ((CP >> 12) & 0x3F)));
    Out.push_back(static_cast<V>(0x80 | ((CP >> 6) & 0x3F)));
    Out.push_back(static_cast<V>(0x80 | (CP & 0x3F)));
  }
}

/// Strip surrounding double quotes from a string literal.
inline std::string_view stripQuotes(std::string_view S) {
  if (S.size() >= 2 && S.front() == '"' && S.back() == '"') {
    return S.substr(1, S.size() - 2);
  }
  return S;
}

/// Strip the leading '$' from a WAT identifier ($abc -> abc, $"abc" -> abc).
/// Escape sequences in quoted identifiers are NOT decoded here; callers needing
/// full normalization should use decodeIdentifier().
inline std::string_view parseIdentifier(std::string_view Text) {
  if (!Text.empty() && Text.front() == '$') {
    auto Inner = Text.substr(1);
    if (Inner.size() >= 2 && Inner.front() == '"' && Inner.back() == '"') {
      return Inner.substr(1, Inner.size() - 2);
    }
    return Inner;
  }
  return Text;
}

/// Decode an identifier's escape sequences into a UTF-8 string suitable for
/// identity comparison (e.g. $"\41B" or $hello). Defined after parseString,
/// which it reuses for escape decoding.
inline Expect<std::string> decodeIdentifier(std::string_view Text);

/// Validate that a byte sequence is valid UTF-8.
inline bool isValidUTF8(const std::string &Data) {
  size_t I = 0;
  while (I < Data.size()) {
    uint8_t B = static_cast<uint8_t>(Data[I]);
    size_t Len = 0;
    uint32_t CodePoint = 0;
    if (B <= 0x7F) {
      ++I;
      continue;
    } else if ((B & 0xE0) == 0xC0) {
      Len = 2;
      CodePoint = B & 0x1F;
      if (CodePoint < 0x02) {
        return false; // overlong
      }
    } else if ((B & 0xF0) == 0xE0) {
      Len = 3;
      CodePoint = B & 0x0F;
    } else if ((B & 0xF8) == 0xF0) {
      Len = 4;
      CodePoint = B & 0x07;
    } else {
      return false; // invalid lead byte
    }
    if (I + Len > Data.size()) {
      return false; // truncated
    }
    for (size_t J = 1; J < Len; ++J) {
      uint8_t C = static_cast<uint8_t>(Data[I + J]);
      if ((C & 0xC0) != 0x80) {
        return false; // invalid continuation
      }
      CodePoint = (CodePoint << 6) | (C & 0x3F);
    }
    if (Len == 2 && CodePoint < 0x80) {
      return false;
    }
    if (Len == 3 && CodePoint < 0x800) {
      return false;
    }
    if (Len == 4 && CodePoint < 0x10000) {
      return false;
    }
    if (CodePoint > 0x10FFFF) {
      return false;
    }
    if (CodePoint >= 0xD800 && CodePoint <= 0xDFFF) {
      return false;
    }
    I += Len;
  }
  return true;
}

/// Remove underscore digit separators from a string.
inline std::string stripUnderscores(std::string_view Text) {
  std::string Result;
  Result.reserve(Text.size());
  for (char C : Text) {
    if (C != '_') {
      Result.push_back(C);
    }
  }
  return Result;
}

/// Numeric base for a WAT literal: only decimal and hex (0x). Leading 0 is NOT
/// octal.
inline int watBase(std::string_view S) {
  if (!S.empty() && (S[0] == '+' || S[0] == '-')) {
    S = S.substr(1);
  }
  if (S.size() >= 2 && S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
    return 16;
  }
  return 10;
}

/// Parse an unsigned integer, handling hex prefix and underscore separators.
inline Expect<uint64_t> parseUint(std::string_view Text) {
  if (Text.empty() || !std::isdigit(static_cast<unsigned char>(Text.front()))) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  const char *Ptr = Cleaned.c_str();
  char *End = nullptr;
  errno = 0;
  uint64_t Val = std::strtoull(Ptr, &End, watBase(Ptr));
  if (End != Ptr + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  if (errno == ERANGE) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  return Val;
}

/// Parse a signed integer, handling sign, hex prefix, and underscore
/// separators. Non-negative values that overflow int64_t but fit uint64_t are
/// returned as reinterpreted bits (e.g. 0xffffffffffffffff for i64.const).
inline Expect<int64_t> parseInt(std::string_view Text) {
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  const char *Ptr = Cleaned.c_str();
  if (*Ptr == '+') {
    ++Ptr;
  }
  int Base = watBase(Ptr);
  char *End = nullptr;
  errno = 0;
  int64_t Val = std::strtoll(Ptr, &End, Base);
  if (End != Cleaned.c_str() + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  if (errno == ERANGE && *Ptr != '-') {
    errno = 0;
    uint64_t UVal = std::strtoull(Ptr, &End, Base);
    if (End != Cleaned.c_str() + Cleaned.size()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    if (errno == ERANGE) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    return static_cast<int64_t>(UVal);
  } else if (errno == ERANGE) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  return Val;
}

/// Parse to uint32_t, reinterpreting negatives. Asserts on invalid input.
inline uint32_t parseU32(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  auto I = static_cast<int32_t>(*R);
  uint32_t V;
  std::memcpy(&V, &I, sizeof(V));
  return V;
}

/// Parse to uint64_t, reinterpreting negatives. Asserts on invalid input.
inline uint64_t parseU64(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  uint64_t V;
  std::memcpy(&V, &*R, sizeof(V));
  return V;
}

/// Parse a float literal: sign, nan:0xN payloads, inf, nan, hex and decimal.
template <typename T, typename UintT>
inline Expect<T> parseFloat(std::string_view Text, UintT ExponentMask,
                            int MantissaBits) {
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  bool Negative = false;
  std::string_view View = Cleaned;
  if (View.front() == '+') {
    View.remove_prefix(1);
  } else if (View.front() == '-') {
    Negative = true;
    View.remove_prefix(1);
  }
  if (View.size() > 6 && View.substr(0, 6) == "nan:0x") {
    std::string_view PayloadHex = View.substr(6);
    if (PayloadHex.empty()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    std::string PayloadStr(PayloadHex);
    char *End = nullptr;
    errno = 0;
    // Range-check the payload at full width before narrowing to UintT, so e.g.
    // f32 nan:0x100000001 is rejected rather than truncated to nan:0x1.
    uint64_t Raw = std::strtoull(PayloadStr.c_str(), &End, 16);
    if (End != PayloadStr.c_str() + PayloadStr.size()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    if (errno == ERANGE) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    if (Raw == 0) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    UintT MantissaMask = (static_cast<UintT>(1) << MantissaBits) - 1;
    if (Raw > static_cast<uint64_t>(MantissaMask)) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    UintT Payload = static_cast<UintT>(Raw);
    UintT Bits = ExponentMask | Payload;
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  }
  // Bare "nan" uses a fixed canonical bit pattern, since strtof/strtod's NaN
  // pattern is implementation-defined (differs between glibc and Windows UCRT).
  if (View == "nan") {
    UintT Bits = ExponentMask | (static_cast<UintT>(1) << (MantissaBits - 1));
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  }
  if (View == "inf") {
    UintT Bits = ExponentMask;
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  }
  const char *Ptr = Cleaned.c_str();
  char *End = nullptr;
  errno = 0;
  T Val;
  if constexpr (std::is_same_v<T, float>) {
    Val = std::strtof(Ptr, &End);
  } else {
    Val = std::strtod(Ptr, &End);
  }
  if (End != Ptr + Cleaned.size()) {
    return Unexpect(End != Ptr ? ErrCode::Value::WatUnknownOperator
                               : ErrCode::Value::WatMalformedNumber);
  }
  if (errno == ERANGE && std::isinf(Val)) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  return Val;
}

inline Expect<float> parseF32(std::string_view Text) {
  return parseFloat<float, uint32_t>(Text, 0x7F800000u, 23);
}

inline Expect<double> parseF64(std::string_view Text) {
  return parseFloat<double, uint64_t>(Text, 0x7FF0000000000000ull, 52);
}

/// Parse f32 to its bit pattern. Assumes valid input.
inline uint32_t parseF32Bits(std::string_view S) {
  auto R = parseF32(S);
  assuming(R.has_value());
  uint32_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Parse f64 to its bit pattern. Assumes valid input.
inline uint64_t parseF64Bits(std::string_view S) {
  auto R = parseF64(S);
  assuming(R.has_value());
  uint64_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Parse a WAT string literal: strip quotes and process escapes (\t, \n, \r,
/// \\, \", \', \xx, \u{xxxx}). Optionally validates UTF-8.
inline Expect<std::string> parseString(std::string_view Text,
                                       bool ValidateUTF8 = true) {
  std::string Result;

  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    Text = Text.substr(1, Text.size() - 2);
  }

  while (!Text.empty()) {
    if (Text.front() != '\\') {
      // stringchar excludes control chars U+00..U+1F and U+7F; they must be
      // escaped, so reject raw control bytes.
      const auto Ch = static_cast<unsigned char>(Text.front());
      if (Ch < 0x20 || Ch == 0x7F) {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Result.push_back(Text.front());
      Text = Text.substr(1);
      continue;
    }
    Text = Text.substr(1); // consume '\'
    if (Text.empty()) {
      break;
    }
    char C = Text.front();
    Text = Text.substr(1); // consume escape char
    switch (C) {
    case 't':
      Result.push_back('\t');
      break;
    case 'n':
      Result.push_back('\n');
      break;
    case 'r':
      Result.push_back('\r');
      break;
    case '"':
      Result.push_back('"');
      break;
    case '\\':
      Result.push_back('\\');
      break;
    case '\'':
      Result.push_back('\'');
      break;
    case 'u': {
      // \u{ hexdigit+ } encodes one Unicode scalar value: requires '{', >=1 hex
      // digit, '}', and a legal scalar (<= U+10FFFF, not a surrogate).
      if (Text.empty() || Text.front() != '{') {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Text = Text.substr(1); // consume '{'
      uint64_t CodePoint = 0;
      size_t Digits = 0;
      while (!Text.empty() && Text.front() != '}') {
        int D = hexDigit(Text.front());
        if (D < 0) {
          return Unexpect(ErrCode::Value::WatMalformedString);
        }
        CodePoint = (CodePoint << 4) | static_cast<uint64_t>(D);
        // Bound the accumulator to prevent overflow on long inputs.
        if (CodePoint > 0x10FFFF) {
          return Unexpect(ErrCode::Value::WatMalformedString);
        }
        ++Digits;
        Text = Text.substr(1);
      }
      if (Digits == 0 || Text.empty() || Text.front() != '}') {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Text = Text.substr(1); // consume '}'
      if (CodePoint >= 0xD800 && CodePoint <= 0xDFFF) {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      encodeUTF8(static_cast<uint32_t>(CodePoint), Result);
      break;
    }
    default: {
      int Hi = hexDigit(C);
      if (Hi >= 0 && !Text.empty()) {
        int Lo = hexDigit(Text.front());
        if (Lo >= 0) {
          Result.push_back(static_cast<char>((Hi << 4) | Lo));
          Text = Text.substr(1);
          break;
        }
      }
      return Unexpect(ErrCode::Value::WatMalformedString);
    }
    }
  }

  if (ValidateUTF8 && !isValidUTF8(Result)) {
    return Unexpect(ErrCode::Value::MalformedUTF8);
  }

  return Result;
}

/// Decode an identifier: quoted ($"...") escapes are decoded via parseString;
/// plain ($abc) names return as-is. Rejects literal control chars or malformed
/// UTF-8 in quoted identifiers.
inline Expect<std::string> decodeIdentifier(std::string_view Text) {
  if (!Text.empty() && Text.front() == '$') {
    Text = Text.substr(1);
  }
  // Bare '$' -> empty identifier
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatEmptyIdentifier);
  }
  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    if (Text.size() == 2) {
      // $"" -> empty identifier
      return Unexpect(ErrCode::Value::WatEmptyIdentifier);
    }
    // Reject literal control chars in the raw source (escapes like \t are fine,
    // but a literal tab/newline is not).
    auto Inner = Text.substr(1, Text.size() - 2);
    for (size_t I = 0; I < Inner.size(); ++I) {
      auto C = static_cast<unsigned char>(Inner[I]);
      if (C < 0x20 || C == 0x7F) {
        return Unexpect(ErrCode::Value::WatEmptyIdentifier);
      }
      // Skip over escape sequences (they start with \).
      if (Inner[I] == '\\' && I + 1 < Inner.size()) {
        ++I; // skip the char after backslash
        if (Inner[I] == 'u' && I + 1 < Inner.size() && Inner[I + 1] == '{') {
          I += 2; // skip \u{...}
          while (I < Inner.size() && Inner[I] != '}') {
            ++I;
          }
        } else if (hexDigit(Inner[I]) >= 0 && I + 1 < Inner.size() &&
                   hexDigit(Inner[I + 1]) >= 0) {
          ++I; // skip second hex digit
        }
      }
    }
    return parseString(Text, true);
  }
  return std::string(Text); // plain identifier: no escapes
}

} // namespace WAT
} // namespace WasmEdge
