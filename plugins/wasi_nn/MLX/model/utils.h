// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include "host/wasi/vfs_io.h"
#include <iostream>
#include <string>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {

std::vector<std::string> splitString(const std::string &S, char Delim);

std::string joinString(std::vector<std::string> &S, char Delim);

bool endsWith(std::string const &Value, std::string const &Ending);

bool startsWith(std::string const &Value, std::string const &Starting);

void saveWeights(const std::unordered_map<std::string, mx::array> &Weights,
                 const std::string Path);

void saveWeights(const mx::array &Weights, const std::string &Path);

std::string loadBytesFromFile(const std::string &Path,
                              const Host::WASI::Environ *Env);

void fillPlaceholders(std::ostringstream &Oss, const std::string &Fmt,
                      size_t &Pos);

template <typename T> std::string toString(const T &Value) {
  std::ostringstream Oss;
  Oss << Value;
  return Oss.str();
}

template <typename T> std::string toString(const std::vector<T> &Vec) {
  std::ostringstream Oss;
  Oss << "[";
  for (size_t I = 0; I < Vec.size(); I++) {
    Oss << toString(Vec[I]);
    if (I + 1 < Vec.size()) {
      Oss << ", ";
    }
  }
  Oss << "]";
  return Oss.str();
}

template <typename T, typename... Args>
void fillPlaceholders(std::ostringstream &Oss, const std::string &Fmt,
                      size_t &Pos, T &&Value, Args &&...args) {
  auto PlaceholderPos = Fmt.find("{}", Pos);
  if (PlaceholderPos == std::string::npos) {
    Oss << Fmt.substr(Pos);
    return;
  }
  Oss << Fmt.substr(Pos, PlaceholderPos - Pos);
  Oss << toString(Value);
  Pos = PlaceholderPos + 2;
  fillPlaceholders(Oss, Fmt, Pos, std::forward<Args>(args)...);
}

template <typename... Args>
std::string formatStr(const std::string &Fmt, Args &&...args) {
  std::ostringstream Oss;
  size_t Pos = 0;
  fillPlaceholders(Oss, Fmt, Pos, std::forward<Args>(args)...);
  return Oss.str();
}

template <typename... Args> void debug(const std::string &fmt, Args &&...args) {
  std::cout << "[DEBUG] " << formatStr(fmt, std::forward<Args>(args)...)
            << std::endl;
}

} // namespace WasmEdge::Host::WASINN::MLX
