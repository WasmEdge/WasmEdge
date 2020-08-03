// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/parser.h - Single Argument parser -------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace SSVM {
namespace PO {

inline void tolower(std::string &String) {
  std::transform(String.begin(), String.end(), String.begin(),
                 [](char c) { return std::tolower(c); });
}

template <typename T> struct Parser;

template <> struct Parser<std::string> {
  static std::string parse(std::string Value) { return Value; }
};

template <> struct Parser<bool> {
  static bool parse(std::string Value) {
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
    throw std::invalid_argument("invalid boolean string: "s + Value);
  }
};

template <> struct Parser<int> {
  static int parse(std::string Value) { return std::stoi(Value); }
};

template <> struct Parser<long> {
  static long parse(std::string Value) { return std::stol(Value); }
};

template <> struct Parser<long long> {
  static long long parse(std::string Value) { return std::stoll(Value); }
};

template <> struct Parser<unsigned long> {
  static unsigned long parse(std::string Value) { return std::stoul(Value); }
};

template <> struct Parser<unsigned long long> {
  static unsigned long long parse(std::string Value) {
    return std::stoull(Value);
  }
};

template <> struct Parser<float> {
  static float parse(std::string Value) { return std::stof(Value); }
};

template <> struct Parser<double> {
  static double parse(std::string Value) { return std::stod(Value); }
};

template <> struct Parser<long double> {
  static long double parse(std::string Value) { return std::stold(Value); }
};

} // namespace PO
} // namespace SSVM
