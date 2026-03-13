// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "converter.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace WasmEdge::WAT {

/// Remove underscore digit separators from a string.
static std::string stripUnderscores(std::string_view Text) {
  std::string Result;
  Result.reserve(Text.size());
  for (char C : Text) {
    if (C != '_') {
      Result.push_back(C);
    }
  }
  return Result;
}

Expect<uint64_t> Converter::parseUint(std::string_view Text) {
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  const char *Ptr = Cleaned.c_str();
  char *End = nullptr;
  errno = 0;
  uint64_t Val = std::strtoull(Ptr, &End, 0);
  if (errno == ERANGE || End != Ptr + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  return Val;
}

Expect<int64_t> Converter::parseInt(std::string_view Text) {
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  // Handle explicit '+' prefix: strtoll doesn't always accept it with hex.
  const char *Ptr = Cleaned.c_str();
  if (*Ptr == '+') {
    ++Ptr;
  }

  char *End = nullptr;
  errno = 0;
  int64_t Val = std::strtoll(Ptr, &End, 0);
  if (errno == ERANGE || End != Cleaned.c_str() + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  return Val;
}

/// Helper: parse a float value from cleaned text.
/// Handles inf, nan, nan:0xN, decimal, and hex float.
template <typename T, typename UintT>
static Expect<T> parseFloat(std::string_view Text, UintT ExponentMask,
                            UintT QuietNaNBit, int MantissaBits) {
  if (Text.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  std::string Cleaned = stripUnderscores(Text);
  if (Cleaned.empty()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }

  // Determine sign.
  bool Negative = false;
  std::string_view View = Cleaned;
  if (View.front() == '+') {
    View.remove_prefix(1);
  } else if (View.front() == '-') {
    Negative = true;
    View.remove_prefix(1);
  }

  // Handle nan:0xN (NaN with payload).
  if (View.size() > 6 && View.substr(0, 6) == "nan:0x") {
    std::string_view PayloadHex = View.substr(6);
    if (PayloadHex.empty()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    char *End = nullptr;
    errno = 0;
    UintT Payload = static_cast<UintT>(
        std::strtoull(std::string(PayloadHex).c_str(), &End, 16));
    if (errno == ERANGE ||
        End != std::string(PayloadHex).c_str() + PayloadHex.size()) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    if (Payload == 0) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    // Ensure payload fits in mantissa bits.
    UintT MantissaMask = (static_cast<UintT>(1) << MantissaBits) - 1;
    if (Payload & ~MantissaMask) {
      return Unexpect(ErrCode::Value::WatMalformedNumber);
    }
    UintT Bits = ExponentMask | QuietNaNBit | Payload;
    if (Negative) {
      Bits |= static_cast<UintT>(1) << (sizeof(T) * 8 - 1);
    }
    T Result;
    std::memcpy(&Result, &Bits, sizeof(T));
    return Result;
  }

  // General case: use strtof/strtod.
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
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  // ERANGE for inf is OK (strtof/strtod returns HUGE_VALF/HUGE_VAL).
  if (errno == ERANGE && !std::isinf(Val)) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  return Val;
}

Expect<float> Converter::parseF32(std::string_view Text) {
  return parseFloat<float, uint32_t>(Text,
                                     /*ExponentMask=*/0x7F800000u,
                                     /*QuietNaNBit=*/0x00400000u,
                                     /*MantissaBits=*/23);
}

Expect<double> Converter::parseF64(std::string_view Text) {
  return parseFloat<double, uint64_t>(Text,
                                      /*ExponentMask=*/0x7FF0000000000000ull,
                                      /*QuietNaNBit=*/0x0008000000000000ull,
                                      /*MantissaBits=*/52);
}

/// Encode a Unicode code point as UTF-8.
static void encodeUTF8(uint32_t CodePoint, std::vector<Byte> &Out) {
  if (CodePoint <= 0x7F) {
    Out.push_back(static_cast<Byte>(CodePoint));
  } else if (CodePoint <= 0x7FF) {
    Out.push_back(static_cast<Byte>(0xC0 | (CodePoint >> 6)));
    Out.push_back(static_cast<Byte>(0x80 | (CodePoint & 0x3F)));
  } else if (CodePoint <= 0xFFFF) {
    Out.push_back(static_cast<Byte>(0xE0 | (CodePoint >> 12)));
    Out.push_back(static_cast<Byte>(0x80 | ((CodePoint >> 6) & 0x3F)));
    Out.push_back(static_cast<Byte>(0x80 | (CodePoint & 0x3F)));
  } else if (CodePoint <= 0x10FFFF) {
    Out.push_back(static_cast<Byte>(0xF0 | (CodePoint >> 18)));
    Out.push_back(static_cast<Byte>(0x80 | ((CodePoint >> 12) & 0x3F)));
    Out.push_back(static_cast<Byte>(0x80 | ((CodePoint >> 6) & 0x3F)));
    Out.push_back(static_cast<Byte>(0x80 | (CodePoint & 0x3F)));
  }
}

/// Parse a hex digit character to its value (0-15). Returns -1 on invalid.
static int hexDigit(char C) {
  if (C >= '0' && C <= '9')
    return C - '0';
  if (C >= 'a' && C <= 'f')
    return C - 'a' + 10;
  if (C >= 'A' && C <= 'F')
    return C - 'A' + 10;
  return -1;
}

std::vector<Byte> Converter::parseString(std::string_view Text) {
  std::vector<Byte> Result;

  // Strip surrounding double quotes.
  if (Text.size() >= 2 && Text.front() == '"' && Text.back() == '"') {
    Text = Text.substr(1, Text.size() - 2);
  }

  size_t I = 0;
  while (I < Text.size()) {
    if (Text[I] == '\\') {
      ++I;
      if (I >= Text.size()) {
        break;
      }
      switch (Text[I]) {
      case 't':
        Result.push_back(0x09);
        ++I;
        break;
      case 'n':
        Result.push_back(0x0A);
        ++I;
        break;
      case 'r':
        Result.push_back(0x0D);
        ++I;
        break;
      case '"':
        Result.push_back(0x22);
        ++I;
        break;
      case '\\':
        Result.push_back(0x5C);
        ++I;
        break;
      case '\'':
        Result.push_back(0x27);
        ++I;
        break;
      case 'u': {
        // \u{xxxx} - Unicode code point.
        ++I; // skip 'u'
        if (I < Text.size() && Text[I] == '{') {
          ++I; // skip '{'
          uint32_t CodePoint = 0;
          while (I < Text.size() && Text[I] != '}') {
            int D = hexDigit(Text[I]);
            if (D < 0)
              break;
            CodePoint = (CodePoint << 4) | static_cast<uint32_t>(D);
            ++I;
          }
          if (I < Text.size() && Text[I] == '}') {
            ++I; // skip '}'
          }
          encodeUTF8(CodePoint, Result);
        }
        break;
      }
      default: {
        // \xx - two hex digits.
        int Hi = hexDigit(Text[I]);
        if (Hi >= 0 && I + 1 < Text.size()) {
          int Lo = hexDigit(Text[I + 1]);
          if (Lo >= 0) {
            Result.push_back(static_cast<Byte>((Hi << 4) | Lo));
            I += 2;
            break;
          }
        }
        // Unknown escape - just emit the character as-is.
        Result.push_back(static_cast<Byte>(Text[I]));
        ++I;
        break;
      }
      }
    } else {
      Result.push_back(static_cast<Byte>(Text[I]));
      ++I;
    }
  }

  return Result;
}

} // namespace WasmEdge::WAT
