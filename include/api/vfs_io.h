// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "host/wasi/environ.h"
#include "wasi/api.hpp"
#include <cctype>
#include <ios>
#include <string>
#include <type_traits>

#if defined(_WIN32)
#ifdef WASMEDGE_COMPILE_LIBRARY
#define WASMEDGE_VFS_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_VFS_EXPORT __declspec(dllimport)
#endif
#else
#define WASMEDGE_VFS_EXPORT __attribute__((visibility("default")))
#endif

namespace WasmEdge {
namespace Host {

namespace API {

class WASMEDGE_VFS_EXPORT WasmEdgeIfstream {
public:
  WasmEdgeIfstream(const Host::WASI::Environ *WASIEnv,
                   const std::string_view &FileName) noexcept;
  ~WasmEdgeIfstream();
  WasmEdgeIfstream(const WasmEdgeIfstream &) = delete;
  WasmEdgeIfstream &operator=(const WasmEdgeIfstream &) = delete;

  bool is_open() const noexcept { return IsOpen; }
  bool good() const noexcept { return !HasError; }
  bool eof() const noexcept { return IsEof; }
  bool fail() const noexcept { return HasError; }
  explicit operator bool() const noexcept { return good(); }

  WasmEdgeIfstream &read(char *Buffer, std::streamsize Count);
  std::streamsize readsome(char *Buffer, std::streamsize Count);
  int get();
  WasmEdgeIfstream &getline(std::string &Line, char Delim = '\n');
  std::string getline(char Delim = '\n');

  template <typename T> WasmEdgeIfstream &operator>>(T &Value);

  std::streampos tellg();
  WasmEdgeIfstream &seekg(std::streampos Pos);
  WasmEdgeIfstream &seekg(std::streamoff Off, std::ios_base::seekdir Way);

private:
  Host::WASI::Environ *Env;
  __wasi_fd_t Fd;
  bool IsOpen;
  bool HasError;
  bool IsEof;

  void setError() { HasError = true; }
};

class WASMEDGE_VFS_EXPORT WasmEdgeOfstream {
public:
  WasmEdgeOfstream(const Host::WASI::Environ *Env,
                   const std::string_view &FileName) noexcept;
  ~WasmEdgeOfstream();
  WasmEdgeOfstream(const WasmEdgeOfstream &) = delete;
  WasmEdgeOfstream &operator=(const WasmEdgeOfstream &) = delete;

  bool is_open() const noexcept { return IsOpen; }
  bool good() const noexcept { return !HasError; }
  bool fail() const noexcept { return HasError; }
  explicit operator bool() const noexcept { return good(); }

  WasmEdgeOfstream &write(const char *Buffer, std::streamsize Count);
  WasmEdgeOfstream &put(char C);
  WasmEdgeOfstream &flush();

  template <typename T> WasmEdgeOfstream &operator<<(const T &Value);

  std::streampos tellp();
  WasmEdgeOfstream &seekp(std::streampos Pos);
  WasmEdgeOfstream &seekp(std::streamoff Off, std::ios_base::seekdir Way);

  void setChunkSize(std::streamsize Size) noexcept { ChunkSize = Size; }
  std::streamsize getChunkSize() const noexcept { return ChunkSize; }

private:
  Host::WASI::Environ *Env;
  __wasi_fd_t Fd;
  bool IsOpen;
  bool HasError;
  std::streamsize ChunkSize;

  void setError() { HasError = true; }
};

template <typename T> WasmEdgeIfstream &WasmEdgeIfstream::operator>>(T &Value) {
  if (!good()) {
    return *this;
  }

  std::string Str;
  char C;

  while ((C = get()) != EOF && std::isspace(C)) {
  }

  if (C == EOF) {
    setError();
    return *this;
  }

  do {
    Str += C;
    C = get();
  } while (C != EOF && !std::isspace(C));

  try {
    if constexpr (std::is_same_v<T, std::string>) {
      Value = Str;
    } else if constexpr (std::is_same_v<T, int>) {
      Value = std::stoi(Str);
    } else if constexpr (std::is_same_v<T, double>) {
      Value = std::stod(Str);
    } else if constexpr (std::is_same_v<T, float>) {
      Value = std::stof(Str);
    } else if constexpr (std::is_same_v<T, long>) {
      Value = std::stol(Str);
    }
  } catch (...) {
    setError();
  }

  return *this;
}

template <typename T>
WasmEdgeOfstream &WasmEdgeOfstream::operator<<(const T &Value) {
  if (!good()) {
    return *this;
  }

  std::string Str;
  if constexpr (std::is_same_v<T, std::string>) {
    Str = Value;
  } else if constexpr (std::is_same_v<T, const char *>) {
    Str = Value;
  } else if constexpr (std::is_arithmetic_v<T>) {
    Str = std::to_string(Value);
  } else {
    Str = std::to_string(Value);
  }

  return write(Str.c_str(), Str.length());
}

} // namespace API
} // namespace Host
} // namespace WasmEdge