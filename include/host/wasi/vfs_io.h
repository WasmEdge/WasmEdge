// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "host/wasi/environ.h"
#include "wasi/api.hpp"
#include <cctype>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>

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
namespace FStream {

class WASMEDGE_VFS_EXPORT IFStream {
public:
  IFStream(const std::string_view FileName,
           const Host::WASI::Environ *WASIEnv = nullptr) noexcept;
  ~IFStream();
  IFStream(const IFStream &) = delete;
  IFStream &operator=(const IFStream &) = delete;

  bool is_open() const noexcept;
  bool good() const noexcept;
  bool eof() const noexcept;
  bool fail() const noexcept;
  explicit operator bool() const noexcept { return good(); }

  IFStream &read(char *Buffer, std::streamsize Count);
  std::streamsize readsome(char *Buffer, std::streamsize Count);
  int get();
  IFStream &getline(std::string &Line, char Delim = '\n');
  std::string getline(char Delim = '\n');

  template <typename T> IFStream &operator>>(T &Value);

  std::streampos tellg();
  IFStream &seekg(std::streampos Pos);
  IFStream &seekg(std::streamoff Off, std::ios_base::seekdir Way);

  void close();

private:
  Host::WASI::Environ *Env;
  __wasi_fd_t Fd;
  bool IsOpen;
  bool HasError;
  bool IsEof;
  bool UseWASI;
  std::ifstream StdStream;

  void setError() { HasError = true; }
};

class WASMEDGE_VFS_EXPORT OFStream {
public:
  OFStream(const std::string_view FileName,
           const Host::WASI::Environ *WASIEnv = nullptr) noexcept;
  OFStream(const std::string_view FileName, std::ios_base::openmode Mode,
           const Host::WASI::Environ *WASIEnv) noexcept;
  ~OFStream();
  OFStream(const OFStream &) = delete;
  OFStream &operator=(const OFStream &) = delete;

  bool is_open() const noexcept;
  bool good() const noexcept;
  bool fail() const noexcept;
  explicit operator bool() const noexcept { return good(); }

  OFStream &write(const char *Buffer, std::streamsize Count);
  OFStream &put(char C);
  OFStream &flush();

  template <typename T> OFStream &operator<<(const T &Value);

  std::streampos tellp();
  OFStream &seekp(std::streampos Pos);
  OFStream &seekp(std::streamoff Off, std::ios_base::seekdir Way);

  void setChunkSize(std::streamsize Size) noexcept { ChunkSize = Size; }
  std::streamsize getChunkSize() const noexcept { return ChunkSize; }

  void close();

private:
  Host::WASI::Environ *Env;
  __wasi_fd_t Fd;
  bool IsOpen;
  bool HasError;
  std::streamsize ChunkSize;
  bool UseWASI;
  std::ofstream StdStream;

  void setError() { HasError = true; }
};

template <typename T> IFStream &IFStream::operator>>(T &Value) {
  if (!good()) {
    return *this;
  }

  if (UseWASI) {
    std::string Str;
    char C;

    while ((C = get()) != EOF && !std::isspace(C)) {
      Str += C;
    }

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
  } else {
    StdStream >> Value;
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

template <typename T> OFStream &OFStream::operator<<(const T &Value) {
  if (!good()) {
    return *this;
  }

  if (UseWASI) {
    std::ostringstream Oss;
    Oss << Value;
    std::string Str = Oss.str();
    return write(Str.c_str(), Str.length());
  } else {
    StdStream << Value;
    if (StdStream.fail()) {
      setError();
    }
    return *this;
  }
}

} // namespace FStream
} // namespace WasmEdge
