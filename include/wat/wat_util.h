// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/types.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace WAT {

/// Parse a float literal with the "C" numeric locale. The strtof and strtod
/// functions read the decimal point of LC_NUMERIC. Then an embedder that calls
/// setlocale on a comma-decimal system cannot load a float literal with them.
/// The text format is locale-independent, as the binary format is. The
/// functions also round to nearest, ties to even, whatever the rounding mode of
/// the caller is.
float strtofC(const char *Ptr, char **End);
double strtodC(const char *Ptr, char **End);
template <typename T> inline T strtoFloatC(const char *Ptr, char **End) {
  if constexpr (std::is_same_v<T, float>) {
    return strtofC(Ptr, End);
  } else {
    return strtodC(Ptr, End);
  }
}

/// Find the value of the keyword Key in Table, a small table of keyword and
/// value pairs. A table of this form needs no dynamic initialization, as a
/// hash map does.
template <typename T, size_t N>
constexpr std::optional<T>
lookupKeyword(const std::pair<std::string_view, T> (&Table)[N],
              std::string_view Key) noexcept {
  for (const auto &[Name, Value] : Table) {
    if (Name == Key) {
      return Value;
    }
  }
  return std::nullopt;
}

/// Return true if Text starts with a UTF-8 byte order mark (BOM). A text file
/// can start with one.
inline bool hasUTF8BOM(std::string_view Text) noexcept {
  return Text.substr(0, 3) == std::string_view("\xEF\xBB\xBF");
}

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

/// Give the length of the UTF-8 sequence at Data[Pos], or 0 if the sequence
/// is not valid. An ASCII byte is a sequence of length 1. The function always
/// examines the lead byte, the length, and the continuation bytes. If
/// CheckScalar is true, the function also rejects an overlong encoding, a
/// code point that is more than U+10FFFF, and a surrogate.
inline size_t utf8SequenceLength(std::string_view Data, size_t Pos,
                                 bool CheckScalar = true) {
  const auto B = static_cast<uint8_t>(Data[Pos]);
  size_t Len = 0;
  uint32_t CodePoint = 0;
  if (B <= 0x7F) {
    return 1;
  }
  if ((B & 0xE0) == 0xC0) {
    Len = 2;
    CodePoint = B & 0x1F;
  } else if ((B & 0xF0) == 0xE0) {
    Len = 3;
    CodePoint = B & 0x0F;
  } else if ((B & 0xF8) == 0xF0) {
    Len = 4;
    CodePoint = B & 0x07;
  } else {
    return 0; // The lead byte is not valid.
  }
  if (Pos + Len > Data.size()) {
    return 0; // The input ends before the sequence ends.
  }
  for (size_t J = 1; J < Len; ++J) {
    const auto C = static_cast<uint8_t>(Data[Pos + J]);
    if ((C & 0xC0) != 0x80) {
      return 0; // The continuation byte is not valid.
    }
    CodePoint = (CodePoint << 6) | (C & 0x3F);
  }
  if (CheckScalar &&
      ((Len == 2 && CodePoint < 0x80) || (Len == 3 && CodePoint < 0x800) ||
       (Len == 4 && CodePoint < 0x10000) || CodePoint > 0x10FFFF ||
       (CodePoint >= 0xD800 && CodePoint <= 0xDFFF))) {
    return 0;
  }
  return Len;
}

/// Examine a byte sequence and find if it is valid UTF-8.
inline bool isValidUTF8(std::string_view Data) {
  size_t I = 0;
  while (I < Data.size()) {
    const size_t Len = utf8SequenceLength(Data, I);
    if (Len == 0) {
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

/// Examine the lexical form of an unsigned integer literal. The grammar is
/// digit ('_'? digit)* or 0x hexdigit ('_'? hexdigit)*, with a lower-case
/// prefix. The lexer applies this grammar to a number token. A numeral inside
/// a keyword token, such as offset=1_0, needs this function.
inline bool isUnsignedLiteral(std::string_view Text) {
  bool IsHex = false;
  if (Text.size() >= 2 && Text[0] == '0' && Text[1] == 'x') {
    IsHex = true;
    Text.remove_prefix(2);
  }
  if (Text.empty()) {
    return false;
  }
  bool PrevDigit = false;
  for (char C : Text) {
    if (C == '_') {
      if (!PrevDigit) {
        return false;
      }
      PrevDigit = false;
      continue;
    }
    if (IsHex ? hexDigit(C) < 0 : !(C >= '0' && C <= '9')) {
      return false;
    }
    PrevDigit = true;
  }
  return PrevDigit;
}

/// Parse an unsigned integer. The function accepts a lower-case 0x hex prefix
/// and underscore separators. A 0 at the start does not make the literal
/// octal. The function does not check the placement of the underscores, so
/// Text must be a token that the lexer or isUnsignedLiteral accepted.
inline Expect<uint64_t> parseUint(std::string_view Text) {
  if (Text.empty() || !std::isdigit(static_cast<unsigned char>(Text.front()))) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  uint64_t Base = 10;
  if (Text.size() >= 2 && Text[0] == '0' && Text[1] == 'x') {
    Base = 16;
    Text.remove_prefix(2);
  }
  uint64_t Val = 0;
  bool HasDigit = false;
  bool Overflow = false;
  for (char C : Text) {
    if (C == '_') {
      continue;
    }
    const int Digit = Base == 16               ? hexDigit(C)
                      : (C >= '0' && C <= '9') ? C - '0'
                                               : -1;
    if (Digit < 0) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    const auto D = static_cast<uint64_t>(Digit);
    if (Val > (std::numeric_limits<uint64_t>::max() - D) / Base) {
      Overflow = true;
    }
    Val = Val * Base + D;
    HasDigit = true;
  }
  if (!HasDigit) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  if (Overflow) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  return Val;
}

/// Parse an unsigned integer that must fit in uint32_t, such as an index. The
/// function accepts a hex prefix and underscore separators.
inline Expect<uint32_t> parseUint32(std::string_view Text) {
  EXPECTED_TRY(auto Val, parseUint(Text));
  if (Val > std::numeric_limits<uint32_t>::max()) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  return static_cast<uint32_t>(Val);
}

/// Parse an integer literal of the width of UT into its two's complement bits.
/// The function accepts a sign, a hex prefix, and underscore separators. The
/// range is the union of the signed and the unsigned ranges of the width. For
/// example, -1 and 0xffffffff both give the bits 0xffffffff for a uint32_t.
template <typename UT> inline Expect<UT> parseInt(std::string_view Text) {
  static_assert(std::is_unsigned_v<UT>);
  const bool Negative = !Text.empty() && Text.front() == '-';
  if (!Text.empty() && (Negative || Text.front() == '+')) {
    Text.remove_prefix(1);
  }
  EXPECTED_TRY(auto Magnitude, parseUint(Text));
  // The magnitude of a negative value can be one more than the maximum of the
  // signed type, for example 128 for -128 in 8 bits.
  const uint64_t Limit =
      Negative ? static_cast<uint64_t>(std::numeric_limits<UT>::max() / 2) + 1
               : static_cast<uint64_t>(std::numeric_limits<UT>::max());
  if (Magnitude > Limit) {
    return Unexpect(ErrCode::Value::WatConstantOutOfRange);
  }
  const auto Bits = static_cast<UT>(Magnitude);
  return Negative ? static_cast<UT>(UT{0} - Bits) : Bits;
}

/// Parse a float literal. The function removes the underscores without a
/// check of their placement, so Text must be a token that the lexer accepted.
/// The function accepts a sign, a nan:0xN payload, inf,
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
  const auto FromBits = [Negative](UintT Bits) {
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  };
  if (View.size() > 6 && View.substr(0, 6) == "nan:0x") {
    // Examine the payload range at full width before the narrow cast to
    // UintT. The parser then rejects f32 nan:0x100000001 and does not
    // truncate it to nan:0x1.
    EXPECTED_TRY(uint64_t Raw, parseUint(View.substr(4)));
    if (Raw == 0) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    UintT MantissaMask = (static_cast<UintT>(1) << MantissaBits) - 1;
    if (Raw > static_cast<uint64_t>(MantissaMask)) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    return FromBits(ExponentMask | static_cast<UintT>(Raw));
  }
  // A bare "nan" uses a fixed canonical bit pattern. The NaN pattern of strtof
  // and strtod is implementation-defined, and it is different between glibc
  // and the Windows UCRT.
  if (View == "nan") {
    return FromBits(ExponentMask |
                    (static_cast<UintT>(1) << (MantissaBits - 1)));
  }
  if (View == "inf") {
    return FromBits(ExponentMask);
  }
  const char *Ptr = Cleaned.c_str();
  char *End = nullptr;
  errno = 0;
  T Val = strtoFloatC<T>(Ptr, &End);
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

/// Parse a float literal into its bit pattern UT. A uint32_t gives an f32, and
/// a uint64_t gives an f64.
template <typename UT> inline Expect<UT> parseFloatBits(std::string_view Text) {
  static_assert(sizeof(UT) == sizeof(float) || sizeof(UT) == sizeof(double));
  UT Bits;
  if constexpr (sizeof(UT) == sizeof(float)) {
    EXPECTED_TRY(auto F, parseF32(Text));
    std::memcpy(&Bits, &F, sizeof(Bits));
  } else {
    EXPECTED_TRY(auto F, parseF64(Text));
    std::memcpy(&Bits, &F, sizeof(Bits));
  }
  return Bits;
}

/// Return true if C is a control character, U+00 to U+1F or U+7F.
inline constexpr bool isControlChar(unsigned char C) noexcept {
  return C < 0x20 || C == 0x7F;
}

/// Return true if C is a WAT whitespace character.
inline constexpr bool isWatSpace(unsigned char C) noexcept {
  return C == ' ' || C == '\t' || C == '\n' || C == '\r';
}

/// Give the end of the line comment that starts at Pos. The end is the
/// position of the first newline character, U+0A or U+0D, or the size of Data
/// if the comment ends the text.
inline size_t lineCommentEnd(std::string_view Data, size_t Pos) noexcept {
  auto End = Data.find_first_of("\n\r", Pos);
  return End == std::string_view::npos ? Data.size() : End;
}

/// Give the position after the ";)" that closes the block comment whose "(;"
/// ends at Pos. A block comment can nest. The result is the size of Data if
/// the comment is not closed.
inline size_t blockCommentEnd(std::string_view Data, size_t Pos) noexcept {
  for (size_t Depth = 1; Pos + 1 < Data.size();) {
    if (Data[Pos] == '(' && Data[Pos + 1] == ';') {
      ++Depth;
      Pos += 2;
    } else if (Data[Pos] == ';' && Data[Pos + 1] == ')') {
      Pos += 2;
      if (--Depth == 0) {
        return Pos;
      }
    } else {
      ++Pos;
    }
  }
  return Data.size();
}

/// Write the bits of lane I into V128. Lane 0 is the least significant lane,
/// as in the binary format, so the value does not depend on the host byte
/// order.
template <typename UT>
inline void setV128Lane(uint128_t &V128, size_t I, UT Bits) noexcept {
  V128 |= static_cast<uint128_t>(static_cast<uint64_t>(Bits))
          << static_cast<unsigned int>(I * sizeof(UT) * 8);
}

/// Return true if C cannot be the first character of an annotation id. The
/// character is a parenthesis or whitespace.
inline constexpr bool isAnnotationIdEnd(unsigned char C) noexcept {
  return C == '(' || C == ')' || isWatSpace(C);
}

/// Parse a WAT string literal. The function removes the quotes and processes
/// the escapes (\t, \n, \r, \\, \", \', \xx, \u{xxxx}). The function can also
/// make sure that the result is valid UTF-8.
inline Expect<std::string> parseString(std::string_view Text,
                                       bool ValidateUTF8 = true) {
  std::string Result;
  Text = stripQuotes(Text);

  while (!Text.empty()) {
    if (Text.front() != '\\') {
      // A stringchar does not include the control characters U+00 to U+1F and
      // U+7F. These characters need an escape, so reject a raw control byte.
      if (isControlChar(static_cast<unsigned char>(Text.front()))) {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Result.push_back(Text.front());
      Text = Text.substr(1);
      continue;
    }
    Text = Text.substr(1); // Consume the backslash.
    if (Text.empty()) {
      return Unexpect(ErrCode::Value::WatMalformedString);
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
      // The form \u{ hexnum } encodes one Unicode scalar value. It needs
      // '{', one hex digit or more, and '}'. A '_' can come between two
      // digits, as in a numeric literal. The scalar must not be more than
      // U+10FFFF, and it must not be a surrogate.
      if (Text.empty() || Text.front() != '{') {
        return Unexpect(ErrCode::Value::WatMalformedString);
      }
      Text = Text.substr(1); // Consume the '{'.
      uint64_t CodePoint = 0;
      size_t Digits = 0;
      bool PrevDigit = false;
      while (!Text.empty() && Text.front() != '}') {
        if (Text.front() == '_' && PrevDigit) {
          PrevDigit = false;
          Text = Text.substr(1);
          continue;
        }
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
        PrevDigit = true;
        Text = Text.substr(1);
      }
      if (Digits == 0 || !PrevDigit || Text.empty() || Text.front() != '}') {
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
    auto Inner = stripQuotes(Text);
    if (std::any_of(Inner.begin(), Inner.end(), [](char C) {
          return isControlChar(static_cast<unsigned char>(C));
        })) {
      return Unexpect(ErrCode::Value::WatEmptyIdentifier);
    }
    return parseString(Text, /*ValidateUTF8=*/true);
  }
  return std::string(Text); // A plain identifier has no escapes.
}

} // namespace WAT
} // namespace WasmEdge
