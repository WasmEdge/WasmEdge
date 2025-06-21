// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "api/vfs_io.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

#include <iostream>

namespace WasmEdge {
namespace Host {
namespace API {

WasmEdgeIfstream::WasmEdgeIfstream(const Host::WASI::Environ *WASIEnv,
                                   const std::string_view &FileName) noexcept
    : Fd(0), IsOpen(false), HasError(false), IsEof(false) {
  Env = const_cast<Host::WASI::Environ *>(WASIEnv);
  __wasi_fd_t BaseFd = 3;

  auto Result = Env->pathOpen(
      BaseFd, FileName, static_cast<__wasi_lookupflags_t>(0),
      __WASI_OFLAGS_CREAT,
      __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL,
      __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));

  if (Result) {
    Fd = *Result;
    IsOpen = true;
  } else {
    HasError = true;
    spdlog::error("Failed to open file for reading: {}", std::string(FileName));
  }
}

WasmEdgeIfstream::~WasmEdgeIfstream() {
  if (IsOpen) {
    Env->fdClose(Fd);
  }
}

WasmEdgeIfstream &WasmEdgeIfstream::read(char *Buffer, std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(Buffer), Count);
  Span<Span<uint8_t>> Iovs(&BufferSpan, 1);
  __wasi_size_t NRead;

  auto Result = Env->fdRead(Fd, Iovs, NRead);
  if (!Result) {
    setError();
  } else if (NRead == 0) {
    IsEof = true;
  }

  return *this;
}

std::streamsize WasmEdgeIfstream::readsome(char *Buffer,
                                           std::streamsize Count) {
  if (!good() || Count <= 0) {
    return 0;
  }

  Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(Buffer), Count);
  Span<Span<uint8_t>> Iovs(&BufferSpan, 1);
  __wasi_size_t NRead;

  auto Result = Env->fdRead(Fd, Iovs, NRead);
  if (!Result) {
    setError();
    return 0;
  }

  if (NRead == 0) {
    IsEof = true;
  }

  return NRead;
}

int WasmEdgeIfstream::get() {
  if (!good()) {
    return EOF;
  }

  char C;
  Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(&C), 1);
  Span<Span<uint8_t>> Iovs(&BufferSpan, 1);
  __wasi_size_t NRead;

  auto Result = Env->fdRead(Fd, Iovs, NRead);
  if (!Result) {
    setError();
    return EOF;
  }

  if (NRead == 0) {
    IsEof = true;
    return EOF;
  }

  return static_cast<unsigned char>(C);
}

WasmEdgeIfstream &WasmEdgeIfstream::getline(std::string &Line, char Delim) {
  Line.clear();
  int C;

  while ((C = get()) != EOF && C != Delim) {
    Line += static_cast<char>(C);
  }

  if (C == EOF && Line.empty()) {
    setError();
  }

  return *this;
}

std::string WasmEdgeIfstream::getline(char Delim) {
  std::string Line;
  getline(Line, Delim);
  return Line;
}

std::streampos WasmEdgeIfstream::tellg() {
  if (!IsOpen) {
    setError();
    return -1;
  }

  __wasi_filesize_t Pos;
  auto Result = Env->fdTell(Fd, Pos);
  if (!Result) {
    setError();
    return -1;
  }

  return static_cast<std::streampos>(Pos);
}

WasmEdgeIfstream &WasmEdgeIfstream::seekg(std::streampos Pos) {
  if (!IsOpen) {
    setError();
    return *this;
  }

  __wasi_filesize_t NewPos;
  auto Result = Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Pos),
                            __WASI_WHENCE_SET, NewPos);
  if (!Result) {
    setError();
  } else {
    IsEof = false;
  }

  return *this;
}

WasmEdgeIfstream &WasmEdgeIfstream::seekg(std::streamoff Off,
                                          std::ios_base::seekdir Way) {
  if (!IsOpen) {
    setError();
    return *this;
  }

  __wasi_whence_t Whence;
  switch (Way) {
  case std::ios_base::beg:
    Whence = __WASI_WHENCE_SET;
    break;
  case std::ios_base::cur:
    Whence = __WASI_WHENCE_CUR;
    break;
  case std::ios_base::end:
    Whence = __WASI_WHENCE_END;
    break;
  default:
    setError();
    return *this;
  }

  __wasi_filesize_t NewPos;
  auto Result =
      Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Off), Whence, NewPos);
  if (!Result) {
    setError();
  } else {
    IsEof = false;
  }

  return *this;
}

WasmEdgeOfstream::WasmEdgeOfstream(const Host::WASI::Environ *WASIEnv,
                                   const std::string_view &FileName) noexcept
    : Fd(0), IsOpen(false), HasError(false), ChunkSize(64 * 1024) {
  Env = const_cast<Host::WASI::Environ *>(WASIEnv);
  __wasi_fd_t BaseFd = 3;

  auto Result = Env->pathOpen(
      BaseFd, FileName, static_cast<__wasi_lookupflags_t>(0),
      __WASI_OFLAGS_CREAT | __WASI_OFLAGS_TRUNC,
      __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL,
      __WASI_RIGHTS_FD_WRITE, static_cast<__wasi_fdflags_t>(0));

  if (Result) {
    Fd = *Result;
    IsOpen = true;
  } else {
    HasError = true;
    spdlog::error("Failed to open file for writing: {}", std::string(FileName));
  }
}

WasmEdgeOfstream::~WasmEdgeOfstream() {
  if (IsOpen) {
    Env->fdClose(Fd);
  }
}

WasmEdgeOfstream &WasmEdgeOfstream::write(const char *Buffer,
                                          std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  std::streamsize Remaining = Count;
  const char *CurrentBuffer = Buffer;

  while (Remaining > 0 && good()) {
    std::streamsize ChunkSizeToWrite = std::min(Remaining, ChunkSize);

    Span<const uint8_t> DataSpan(
        reinterpret_cast<const uint8_t *>(CurrentBuffer), ChunkSizeToWrite);
    Span<Span<const uint8_t>> Iovs(&DataSpan, 1);
    __wasi_size_t NWritten;

    auto Result = Env->fdWrite(Fd, Iovs, NWritten);
    if (!Result) {
      setError();
      break;
    }

    if (NWritten != static_cast<__wasi_size_t>(ChunkSizeToWrite)) {
      if (NWritten > 0) {
        CurrentBuffer += NWritten;
        Remaining -= NWritten;
      } else {
        setError();
        break;
      }
    } else {
      CurrentBuffer += ChunkSizeToWrite;
      Remaining -= ChunkSizeToWrite;
    }
  }

  return *this;
}

WasmEdgeOfstream &WasmEdgeOfstream::put(char C) { return write(&C, 1); }

WasmEdgeOfstream &WasmEdgeOfstream::flush() { return *this; }

std::streampos WasmEdgeOfstream::tellp() {
  if (!IsOpen) {
    setError();
    return -1;
  }

  __wasi_filesize_t Pos;
  auto Result = Env->fdTell(Fd, Pos);
  if (!Result) {
    setError();
    return -1;
  }

  return static_cast<std::streampos>(Pos);
}

WasmEdgeOfstream &WasmEdgeOfstream::seekp(std::streampos Pos) {
  if (!IsOpen) {
    setError();
    return *this;
  }

  __wasi_filesize_t NewPos;
  auto Result = Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Pos),
                            __WASI_WHENCE_SET, NewPos);
  if (!Result) {
    setError();
  }

  return *this;
}

WasmEdgeOfstream &WasmEdgeOfstream::seekp(std::streamoff Off,
                                          std::ios_base::seekdir Way) {
  if (!IsOpen) {
    setError();
    return *this;
  }

  __wasi_whence_t Whence;
  switch (Way) {
  case std::ios_base::beg:
    Whence = __WASI_WHENCE_SET;
    break;
  case std::ios_base::cur:
    Whence = __WASI_WHENCE_CUR;
    break;
  case std::ios_base::end:
    Whence = __WASI_WHENCE_END;
    break;
  default:
    setError();
    return *this;
  }

  __wasi_filesize_t NewPos;
  auto Result =
      Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Off), Whence, NewPos);
  if (!Result) {
    setError();
  }

  return *this;
}

} // namespace API
} // namespace Host
} // namespace WasmEdge