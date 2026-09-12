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

/// Parse a hex digit into its value (0-15). If the digit is not valid, the
/// function returns -1.
inline int hexDigit(char C) {
  if (C >= '0' && C <= '9')
    return C - '0';
  if (C >= 'a' && C <= 'f')
    return C - 'a' + 10;
  if (C >= 'A' && C <= 'F')
    return C - 'A' + 10;
  return -1;
}

/// Encode a code point as UTF-8 into a container. The function ignores a code
/// point that is more than U+10FFFF.
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

/// Remove the double quotes at the two ends of a string literal.
inline std::string_view stripQuotes(std::string_view S) {
  if (S.size() >= 2 && S.front() == '"' && S.back() == '"') {
    return S.substr(1, S.size() - 2);
  }
  return S;
}

/// Remove the '$' at the start of a WAT identifier ($abc -> abc,
/// $"abc" -> abc). This function does not decode the escape sequences of a
/// quoted identifier. If you need the full normalization, use
/// decodeIdentifier().
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

/// Decode the escape sequences of an identifier into a UTF-8 string. The
/// caller can then compare two identities, for example $"\41B" or $hello.
/// The definition comes after parseString, because this function uses
/// parseString to decode the escapes.
inline Expect<std::string> decodeIdentifier(std::string_view Text);

/// Examine a byte sequence and find if it is valid UTF-8.
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
        return false; // The encoding is overlong.
      }
    } else if ((B & 0xF0) == 0xE0) {
      Len = 3;
      CodePoint = B & 0x0F;
    } else if ((B & 0xF8) == 0xF0) {
      Len = 4;
      CodePoint = B & 0x07;
    } else {
      return false; // The lead byte is not valid.
    }
    if (I + Len > Data.size()) {
      return false; // The sequence is truncated.
    }
    for (size_t J = 1; J < Len; ++J) {
      uint8_t C = static_cast<uint8_t>(Data[I + J]);
      if ((C & 0xC0) != 0x80) {
        return false; // The continuation byte is not valid.
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

/// Give the numeric base of a WAT literal. Only decimal and hex (0x) are
/// possible. A 0 at the start does not make the literal octal.
inline int watBase(std::string_view S) {
  if (!S.empty() && (S[0] == '+' || S[0] == '-')) {
    S = S.substr(1);
  }
  if (S.size() >= 2 && S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
    return 16;
  }
  return 10;
}

/// Parse an unsigned integer. The function accepts a hex prefix and underscore
/// separators.
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

/// Parse a signed integer. The function accepts a sign, a hex prefix, and
/// underscore separators. A value that is not negative can be too large for
/// int64_t but small enough for uint64_t. The function returns such a value as
/// reinterpreted bits, for example 0xffffffffffffffff for i64.const.
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

/// Parse a literal into a uint32_t and reinterpret a negative value. The
/// function asserts if the input is not valid.
inline uint32_t parseU32(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  auto I = static_cast<int32_t>(*R);
  uint32_t V;
  std::memcpy(&V, &I, sizeof(V));
  return V;
}

/// Parse a literal into a uint64_t and reinterpret a negative value. The
/// function asserts if the input is not valid.
inline uint64_t parseU64(std::string_view S) {
  auto R = parseInt(S);
  assuming(R.has_value());
  uint64_t V;
  std::memcpy(&V, &*R, sizeof(V));
  return V;
}

/// Parse a float literal. The function accepts a sign, a nan:0xN payload, inf,
/// nan, hex, and decimal.
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
    // Examine the payload range at full width before the narrow cast to
    // UintT. The parser then rejects f32 nan:0x100000001 and does not
    // truncate it to nan:0x1.
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
  // A bare "nan" uses a fixed canonical bit pattern. The NaN pattern of strtof
  // and strtod is implementation-defined, and it is different between glibc
  // and the Windows UCRT.
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

/// Parse an f32 literal into its bit pattern. The input must be valid.
inline uint32_t parseF32Bits(std::string_view S) {
  auto R = parseF32(S);
  assuming(R.has_value());
  uint32_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Parse an f64 literal into its bit pattern. The input must be valid.
inline uint64_t parseF64Bits(std::string_view S) {
  auto R = parseF64(S);
  assuming(R.has_value());
  uint64_t Bits;
  std::memcpy(&Bits, &*R, sizeof(Bits));
  return Bits;
}

/// Parse a WAT string literal. The function removes the quotes and processes
/// the escapes (\t, \n, \r, \\, \", \', \xx, \u{xxxx}). The function can also
/// make sure that the result is valid UTF-8.
inline Expect<std::string> parseString(std::string_view Text,
                                       bool ValidateUTF8 = true) {
  std::string Result;

  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    Text = Text.substr(1, Text.size() - 2);
  }

  while (!Text.empty()) {
    if (Text.front() != '\\') {
      // A stringchar does not include the control characters U+00 to U+1F and
      // U+7F. These characters need an escape, so reject a raw control byte.
      const auto Ch = static_cast<unsigned char>(Text.front());
      if (Ch < 0x20 || Ch == 0x7F) {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Result.push_back(Text.front());
      Text = Text.substr(1);
      continue;
    }
    Text = Text.substr(1); // Consume the backslash.
    if (Text.empty()) {
      break;
    }
    char C = Text.front();
    Text = Text.substr(1); // Consume the escape character.
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
      // The form \u{ hexdigit+ } encodes one Unicode scalar value. It needs
      // '{', one hex digit or more, and '}'. The scalar must not be more than
      // U+10FFFF, and it must not be a surrogate.
      if (Text.empty() || Text.front() != '{') {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Text = Text.substr(1); // Consume the '{'.
      uint64_t CodePoint = 0;
      size_t Digits = 0;
      while (!Text.empty() && Text.front() != '}') {
        int D = hexDigit(Text.front());
        if (D < 0) {
          return Unexpect(ErrCode::Value::WatMalformedString);
        }
        CodePoint = (CodePoint << 4) | static_cast<uint64_t>(D);
        // Bound the accumulator to prevent an overflow on a long input.
        if (CodePoint > 0x10FFFF) {
          return Unexpect(ErrCode::Value::WatMalformedString);
        }
        ++Digits;
        Text = Text.substr(1);
      }
      if (Digits == 0 || Text.empty() || Text.front() != '}') {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Text = Text.substr(1); // Consume the '}'.
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

/// Decode an identifier. For a quoted identifier ($"..."), parseString decodes
/// the escapes. For a plain identifier ($abc), the function returns the name
/// without a change. The function rejects a literal control character or
/// malformed UTF-8 in a quoted identifier.
inline Expect<std::string> decodeIdentifier(std::string_view Text) {
  if (!Text.empty() && Text.front() == '$') {
    Text = Text.substr(1);
  }
  // A bare '$' gives an empty identifier.
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatEmptyIdentifier);
  }
  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    if (Text.size() == 2) {
      // A $"" gives an empty identifier.
      return Unexpect(ErrCode::Value::WatEmptyIdentifier);
    }
    // Reject a literal control character in the raw source. An escape such as
    // \t is permitted, but a literal tab or newline is not permitted.
    auto Inner = Text.substr(1, Text.size() - 2);
    for (size_t I = 0; I < Inner.size(); ++I) {
      auto C = static_cast<unsigned char>(Inner[I]);
      if (C < 0x20 || C == 0x7F) {
        return Unexpect(ErrCode::Value::WatEmptyIdentifier);
      }
      // Skip the escape sequences. An escape sequence starts with a backslash.
      if (Inner[I] == '\\' && I + 1 < Inner.size()) {
        ++I; // Skip the character after the backslash.
        if (Inner[I] == 'u' && I + 1 < Inner.size() && Inner[I + 1] == '{') {
          I += 2; // Skip the \u{...} form.
          while (I < Inner.size() && Inner[I] != '}') {
            ++I;
          }
        } else if (hexDigit(Inner[I]) >= 0 && I + 1 < Inner.size() &&
                   hexDigit(Inner[I + 1]) >= 0) {
          ++I; // Skip the second hex digit.
        }
      }
    }
    return parseString(Text, true);
  }
  return std::string(Text); // A plain identifier has no escapes.
}

} // namespace WAT
} // namespace WasmEdge
