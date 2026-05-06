// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

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

namespace WasmEdge::WAT {

/// Parse a hex digit character to its value (0-15). Returns -1 on invalid.
inline int hexDigit(char C) {
  if (C >= '0' && C <= '9')
    return C - '0';
  if (C >= 'a' && C <= 'f')
    return C - 'a' + 10;
  if (C >= 'A' && C <= 'F')
    return C - 'A' + 10;
  return -1;
}

/// Encode a Unicode code point as UTF-8 into a container (vector<Byte> or
/// string). Code points above U+10FFFF are silently ignored.
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

/// Strip the leading '$' from a WAT identifier.
/// Returns the identifier name without the '$' prefix.
/// For plain identifiers ($abc), returns "abc".
/// For quoted identifiers ($"abc"), returns "abc" (strips quotes).
/// Note: Escape sequences in quoted identifiers are NOT decoded here;
/// callers needing full normalization should use decodeQuotedIdentifier().
inline std::string_view parseIdentifier(std::string_view Text) {
  if (!Text.empty() && Text.front() == '$') {
    auto Inner = Text.substr(1);
    // Quoted identifier: $"..."
    if (Inner.size() >= 2 && Inner.front() == '"' && Inner.back() == '"') {
      return Inner.substr(1, Inner.size() - 2);
    }
    return Inner;
  }
  return Text;
}

/// Decode a quoted identifier's escape sequences into a UTF-8 string.
/// Input is the raw text of the identifier node (e.g. $"\41B" or $hello).
/// Returns the decoded name suitable for identity comparison.
/// Reuses parseString for escape decoding (declared later in this header).
inline Expect<std::string> decodeIdentifier(std::string_view Text);
// Defined after parseString.

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

/// Determine the numeric base for a WAT literal.
/// WAT only has decimal and hex (0x prefix). Leading 0 is NOT octal.
inline int watBase(std::string_view S) {
  if (!S.empty() && (S[0] == '+' || S[0] == '-')) {
    S = S.substr(1);
  }
  if (S.size() >= 2 && S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
    return 16;
  }
  return 10;
}

/// Parse an unsigned integer string. Returns Expect<uint64_t>.
/// Handles hex prefix, underscore separators, and validates format.
inline Expect<uint64_t> parseUint(std::string_view Text) {
  if (Text.empty() || !std::isdigit(Text.front())) {
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

/// Parse a signed integer string. Returns Expect<int64_t>.
/// Handles sign, hex prefix, underscore separators, and validates format.
/// For non-negative values that overflow int64_t but fit uint64_t, returns
/// the reinterpreted bits (e.g. 0xffffffffffffffff for i64.const).
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

/// Convenience: parse to uint32_t, reinterpreting negative values.
/// Assumes valid input (asserts on error).
inline uint32_t parseU32(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  auto I = static_cast<int32_t>(*R);
  uint32_t V;
  std::memcpy(&V, &I, sizeof(V));
  return V;
}

/// Convenience: parse to uint64_t, reinterpreting negative values.
/// Assumes valid input (asserts on error).
inline uint64_t parseU64(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  uint64_t V;
  std::memcpy(&V, &*R, sizeof(V));
  return V;
}

/// Parse a float literal string. Returns Expect<T>.
/// Handles sign, nan:0xN payloads, inf, nan, hex float, and decimal float.
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
    UintT Payload =
        static_cast<UintT>(std::strtoull(PayloadStr.c_str(), &End, 16));
    if (End != PayloadStr.c_str() + PayloadStr.size()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    if (errno == ERANGE) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    if (Payload == 0) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    UintT MantissaMask = (static_cast<UintT>(1) << MantissaBits) - 1;
    if (Payload & ~MantissaMask) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    UintT Bits = ExponentMask | Payload;
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  }
  // Bare "nan" produces the canonical NaN with a fixed bit pattern, since
  // strtof/strtod's NaN bit pattern is implementation-defined and differs
  // between glibc and Windows UCRT.
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

/// Convenience: parse f32 to bit pattern. Assumes valid input.
inline uint32_t parseF32Bits(std::string_view S) {
  auto R = parseF32(S);
  assuming(R.has_value());
  uint32_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Convenience: parse f64 to bit pattern. Assumes valid input.
inline uint64_t parseF64Bits(std::string_view S) {
  auto R = parseF64(S);
  assuming(R.has_value());
  uint64_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Parse a WAT string literal, processing escape sequences.
/// Strips surrounding quotes, handles \t, \n, \r, \\, \", \', \xx, \u{xxxx}.
/// Optionally validates UTF-8.
inline Expect<std::string> parseString(std::string_view Text,
                                       bool ValidateUTF8 = true) {
  std::string Result;

  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    Text = Text.substr(1, Text.size() - 2);
  }

  while (!Text.empty()) {
    if (Text.front() != '\\') {
      Result.push_back(Text.front());
      Text = Text.substr(1);
      continue;
    }
    Text = Text.substr(1); // consume '\\'
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
      if (Text.size() >= 2 && Text.front() == '{') {
        Text = Text.substr(1); // consume '{'
        uint32_t CodePoint = 0;
        while (!Text.empty() && Text.front() != '}') {
          int D = hexDigit(Text.front());
          if (D < 0)
            break;
          CodePoint = (CodePoint << 4) | static_cast<uint32_t>(D);
          Text = Text.substr(1);
        }
        if (!Text.empty() && Text.front() == '}') {
          Text = Text.substr(1); // consume '}'
        }
        encodeUTF8(CodePoint, Result);
      }
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

/// Decode a quoted identifier ($"...") by reusing parseString for escapes.
/// For plain identifiers ($abc), returns the name as-is.
/// Rejects quoted identifiers with literal control characters or malformed
/// UTF-8.
inline Expect<std::string> decodeIdentifier(std::string_view Text) {
  if (!Text.empty() && Text.front() == '$') {
    Text = Text.substr(1);
  }
  // Bare '$' or '$""' → empty identifier
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatEmptyIdentifier);
  }
  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    if (Text.size() == 2) {
      // $"" → empty identifier
      return Unexpect(ErrCode::Value::WatEmptyIdentifier);
    }
    // Check for literal control characters in the raw source
    // (escape sequences like \t are fine, but literal tab/newline are not)
    auto Inner = Text.substr(1, Text.size() - 2);
    for (size_t I = 0; I < Inner.size(); ++I) {
      auto C = static_cast<unsigned char>(Inner[I]);
      if (C < 0x20 || C == 0x7F) {
        return Unexpect(ErrCode::Value::WatEmptyIdentifier);
      }
      // Skip over escape sequences (they start with \)
      if (Inner[I] == '\\' && I + 1 < Inner.size()) {
        ++I; // skip the char after backslash
        if (Inner[I] == 'u' && I + 1 < Inner.size() && Inner[I + 1] == '{') {
          // Skip \u{...}
          I += 2;
          while (I < Inner.size() && Inner[I] != '}') {
            ++I;
          }
        } else if (hexDigit(Inner[I]) >= 0 && I + 1 < Inner.size() &&
                   hexDigit(Inner[I + 1]) >= 0) {
          ++I; // skip second hex digit
        }
      }
    }
    // Quoted identifier: decode escapes, validate UTF-8
    return parseString(Text, true);
  }
  // Plain identifier: no escapes
  return std::string(Text);
}

} // namespace WasmEdge::WAT
