// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/po/parser.h - Single Argument parser ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/error.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace WasmEdge {
namespace PO {

inline void tolower(std::string &String) noexcept {
  std::transform(
      String.begin(), String.end(), String.begin(),
      [](char C) noexcept { return static_cast<char>(std::tolower(C)); });
}

template <typename ConvResultT, typename ResultT = ConvResultT>
inline cxx20::expected<ResultT, Error>
stringToInteger(ConvResultT (&Conv)(const char *, char **, int),
                std::string Value) noexcept {
  using namespace std::literals;
  char *EndPtr;
  const char *CStr = Value.c_str();
  auto SavedErrNo = std::exchange(errno, 0);
  const auto Result = Conv(CStr, &EndPtr, 10);
  std::swap(SavedErrNo, errno);
  if (EndPtr == CStr) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                    ""s);
  }
  auto InsideRange = [](auto WiderResult) constexpr noexcept {
    using WiderResultT = decltype(WiderResult);
    if constexpr (std::is_same_v<ResultT, WiderResultT>) {
      return true;
    } else {
      return static_cast<WiderResultT>(std::numeric_limits<ResultT>::min()) <=
                 WiderResult &&
             WiderResult <=
                 static_cast<WiderResultT>(std::numeric_limits<ResultT>::max());
    }
  };
  if (SavedErrNo == ERANGE || !InsideRange(Result)) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::OutOfRange, ""s);
  }
  return static_cast<ResultT>(Result);
}

template <typename ConvResultT, typename ResultT = ConvResultT>
inline cxx20::expected<ResultT, Error>
stringToFloating(ConvResultT (&Conv)(const char *, char **),
                 std::string Value) noexcept {
  using namespace std::literals;
  char *EndPtr;
  const char *CStr = Value.c_str();
  auto SavedErrNo = std::exchange(errno, 0);
  const auto Result = Conv(CStr, &EndPtr);
  std::swap(SavedErrNo, errno);
  if (EndPtr == CStr) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                    ""s);
  }
  if (SavedErrNo == ERANGE) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::OutOfRange, ""s);
  }
  return Result;
}

template <typename T> struct Parser;

template <> struct Parser<std::string> {
  static cxx20::expected<std::string, Error> parse(std::string Value) noexcept {
    return Value;
  }
};

template <> struct Parser<bool> {
  static cxx20::expected<bool, Error> parse(std::string Value) noexcept {
    using namespace std::literals;
    if (!Value.empty()) {
      switch (Value[0]) {
      case 'T':
      case 't':
        if (Value.size() == 4) {
          tolower(Value);
          if (Value == "true"sv) {
            return true;
          }
        }
        break;
      case '1':
        if (Value.size() == 1) {
          return true;
        }
        break;
      case 'F':
      case 'f':
        if (Value.size() == 5) {
          tolower(Value);
          if (Value == "false"sv) {
            return false;
          }
        }
        break;
      case '0':
        if (Value.size() == 1) {
          return false;
        }
        break;
      default:
        break;
      }
    }
    return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                    "invalid boolean string: "s + Value);
  }
};

template <> struct Parser<int> {
  static cxx20::expected<int, Error> parse(std::string Value) noexcept {
    return stringToInteger<long, int>(std::strtol, std::move(Value));
  }
};

template <> struct Parser<unsigned int> {
  static cxx20::expected<unsigned int, Error>
  parse(std::string Value) noexcept {
    return stringToInteger<unsigned long, unsigned int>(std::strtoul,
                                                        std::move(Value));
  }
};

template <> struct Parser<signed char> {
  static cxx20::expected<signed char, Error> parse(std::string Value) noexcept {
    return stringToInteger<long, signed char>(std::strtol, std::move(Value));
  }
};

template <> struct Parser<unsigned char> {
  static cxx20::expected<unsigned char, Error>
  parse(std::string Value) noexcept {
    return stringToInteger<unsigned long, unsigned char>(std::strtoul,
                                                         std::move(Value));
  }
};

template <> struct Parser<short> {
  static cxx20::expected<short, Error> parse(std::string Value) noexcept {
    return stringToInteger<long, short>(std::strtol, std::move(Value));
  }
};

template <> struct Parser<unsigned short> {
  static cxx20::expected<unsigned short, Error>
  parse(std::string Value) noexcept {
    return stringToInteger<unsigned long, unsigned short>(std::strtoul,
                                                          std::move(Value));
  }
};

template <> struct Parser<long> {
  static cxx20::expected<long, Error> parse(std::string Value) noexcept {
    return stringToInteger(std::strtol, std::move(Value));
  }
};

template <> struct Parser<long long> {
  static cxx20::expected<long long, Error> parse(std::string Value) noexcept {
    return stringToInteger(std::strtoll, std::move(Value));
  }
};

template <> struct Parser<unsigned long> {
  static cxx20::expected<unsigned long, Error>
  parse(std::string Value) noexcept {
    return stringToInteger(std::strtoul, std::move(Value));
  }
};

template <> struct Parser<unsigned long long> {
  static cxx20::expected<unsigned long long, Error>
  parse(std::string Value) noexcept {
    return stringToInteger(std::strtoull, std::move(Value));
  }
};

template <> struct Parser<float> {
  static cxx20::expected<float, Error> parse(std::string Value) noexcept {
    return stringToFloating(std::strtof, std::move(Value));
  }
};

template <> struct Parser<double> {
  static cxx20::expected<double, Error> parse(std::string Value) noexcept {
    return stringToFloating(std::strtod, std::move(Value));
  }
};

template <> struct Parser<long double> {
  static cxx20::expected<long double, Error> parse(std::string Value) noexcept {
    return stringToFloating(std::strtold, std::move(Value));
  }
};

} // namespace PO
} // namespace WasmEdge
