// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "host/wasi/vfs_io.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

#include <iostream>

namespace WasmEdge {
namespace FStream {
using namespace std::literals;

IFStream::IFStream(const std::string_view FileName,
                   const Host::WASI::Environ *WASIEnv) noexcept
    : Fd(0), IsOpen(false), HasError(false), IsEof(false),
      UseWASI(WASIEnv != nullptr) {

  if (UseWASI) {
    Env = const_cast<Host::WASI::Environ *>(WASIEnv);
    // Using the first default preopened FD 3 as the working directory.
    __wasi_fd_t BaseFd = 3;

    auto ExistsResult = Env->pathExists(FileName);
    if (!ExistsResult || !*ExistsResult) {
      HasError = true;
      IsOpen = false;
      return;
    }

    auto Result = Env->pathOpen(
        BaseFd, FileName, static_cast<__wasi_lookupflags_t>(0),
        static_cast<__wasi_oflags_t>(0),
        __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL,
        __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));

    if (Result) {
      Fd = *Result;
      IsOpen = true;
    } else {
      HasError = true;
      IsOpen = false;
      spdlog::error("Failed to open file for reading: {}"sv,
                    std::string(FileName));
    }
  } else {
    StdStream.open(std::string(FileName), std::ios::in);
    IsOpen = StdStream.is_open();
    if (!IsOpen) {
      HasError = true;
    }
  }
}

IFStream::~IFStream() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
  }
}

bool IFStream::is_open() const noexcept {
  if (UseWASI) {
    return IsOpen;
  }
  return StdStream.is_open();
}

bool IFStream::good() const noexcept {
  if (UseWASI) {
    return !HasError;
  }
  return StdStream.good();
}

bool IFStream::eof() const noexcept {
  if (UseWASI) {
    return IsEof;
  }
  return StdStream.eof();
}

bool IFStream::fail() const noexcept {
  if (UseWASI) {
    return HasError;
  }
  return StdStream.fail();
}

IFStream &IFStream::read(char *Buffer, std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  if (UseWASI) {
    Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(Buffer),
                             static_cast<size_t>(Count));
    Span<Span<uint8_t>> Iovs(&BufferSpan, 1);
    __wasi_size_t NRead;

    auto Result = Env->fdRead(Fd, Iovs, NRead);
    if (!Result) {
      setError();
    } else if (NRead == 0) {
      IsEof = true;
    }
  } else {
    StdStream.read(Buffer, Count);
    if (StdStream.fail() && !StdStream.eof()) {
      setError();
    }
  }

  return *this;
}

std::streamsize IFStream::readsome(char *Buffer, std::streamsize Count) {
  if (!good() || Count <= 0) {
    return 0;
  }

  if (UseWASI) {
    Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(Buffer),
                             static_cast<size_t>(Count));
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
  std::streamsize Result = StdStream.readsome(Buffer, Count);
  if (StdStream.fail()) {
    setError();
    return 0;
  }
  return Result;
}

int IFStream::get() {
  if (!good()) {
    return EOF;
  }

  if (UseWASI) {
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
  int C = StdStream.get();
  if (StdStream.fail() && !StdStream.eof()) {
    setError();
  }
  return C;
}

IFStream &IFStream::getline(std::string &Line, char Delim) {
  Line.clear();

  if (UseWASI) {
    int C;
    while ((C = get()) != EOF && C != Delim) {
      Line += static_cast<char>(C);
    }
    if (Delim == '\n' && !Line.empty() && Line.back() == '\r') {
      Line.pop_back();
    }
  } else {
    std::getline(StdStream, Line, Delim);
  }

  return *this;
}

std::string IFStream::getline(char Delim) {
  std::string Line;
  getline(Line, Delim);
  return Line;
}

std::streampos IFStream::tellg() {
  if (!is_open()) {
    setError();
    return -1;
  }

  if (UseWASI) {
    __wasi_filesize_t Pos;
    auto Result = Env->fdTell(Fd, Pos);
    if (!Result) {
      setError();
      return -1;
    }

    return static_cast<std::streampos>(static_cast<std::streamoff>(Pos));
  }
  std::streampos Pos = StdStream.tellg();
  if (StdStream.fail()) {
    setError();
    return -1;
  }
  return Pos;
}

IFStream &IFStream::seekg(std::streampos Pos) {
  if (!is_open()) {
    setError();
    return *this;
  }

  if (UseWASI) {
    __wasi_filesize_t NewPos;
    auto Result = Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Pos),
                              __WASI_WHENCE_SET, NewPos);
    if (!Result) {
      setError();
    } else {
      IsEof = false;
    }
  } else {
    StdStream.seekg(Pos);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

IFStream &IFStream::seekg(std::streamoff Off, std::ios_base::seekdir Way) {
  if (!is_open()) {
    setError();
    return *this;
  }

  if (UseWASI) {
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
  } else {
    StdStream.seekg(Off, Way);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

void IFStream::close() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
    IsOpen = false;
  }
}

OFStream::OFStream(const std::string_view FileName,
                   const Host::WASI::Environ *WASIEnv) noexcept
    : OFStream(FileName, std::ios_base::out, WASIEnv) {}

OFStream::OFStream(const std::string_view FileName,
                   std::ios_base::openmode Mode,
                   const Host::WASI::Environ *WASIEnv) noexcept
    : Fd(0), IsOpen(false), HasError(false), ChunkSize(64 * 1024),
      UseWASI(WASIEnv != nullptr) {

  if (UseWASI) {
    Env = const_cast<Host::WASI::Environ *>(WASIEnv);
    // Using the first default preopened FD 3 as the working directory.
    __wasi_fd_t BaseFd = 3;

    __wasi_oflags_t OpenFlags = __WASI_OFLAGS_CREAT;
    __wasi_rights_t Rights =
        __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL;

    if (Mode & std::ios::trunc) {
      OpenFlags |= __WASI_OFLAGS_TRUNC;
    }
    auto Result = Env->pathOpen(
        BaseFd, FileName, static_cast<__wasi_lookupflags_t>(0), OpenFlags,
        Rights, Rights, static_cast<__wasi_fdflags_t>(0));

    if (Result) {
      Fd = *Result;
      IsOpen = true;
      if (Mode & std::ios::ate) {
        __wasi_filesize_t NewPos;
        Env->fdSeek(Fd, 0, __WASI_WHENCE_END, NewPos);
      }
    } else {
      HasError = true;
      spdlog::error("Failed to open file for writing: {}",
                    std::string(FileName));
    }
  } else {
    StdStream.open(std::string(FileName), Mode);
    IsOpen = StdStream.is_open();
    if (!IsOpen) {
      HasError = true;
    }
  }
}

OFStream::~OFStream() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
  }
}

bool OFStream::is_open() const noexcept {
  if (UseWASI) {
    return IsOpen;
  }
  return StdStream.is_open();
}

bool OFStream::good() const noexcept {
  if (UseWASI) {
    return !HasError;
  }
  return StdStream.good();
}

bool OFStream::fail() const noexcept {
  if (UseWASI) {
    return HasError;
  }
  return StdStream.fail();
}

OFStream &OFStream::write(const char *Buffer, std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  if (UseWASI) {
    std::streamsize Remaining = Count;
    const char *CurrentBuffer = Buffer;

    while (Remaining > 0 && good()) {
      std::streamsize ChunkSizeToWrite = std::min(Remaining, ChunkSize);

      Span<const uint8_t> DataSpan(
          reinterpret_cast<const uint8_t *>(CurrentBuffer),
          static_cast<size_t>(ChunkSizeToWrite));
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
          Remaining -= static_cast<std::streamsize>(NWritten);
        } else {
          setError();
          break;
        }
      } else {
        CurrentBuffer += ChunkSizeToWrite;
        Remaining -= ChunkSizeToWrite;
      }
    }
  } else {
    StdStream.write(Buffer, Count);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

OFStream &OFStream::put(char C) { return write(&C, 1); }

OFStream &OFStream::flush() {
  if (!UseWASI) {
    StdStream.flush();
    if (StdStream.fail()) {
      setError();
    }
  }
  return *this;
}

std::streampos OFStream::tellp() {
  if (!is_open()) {
    setError();
    return -1;
  }

  if (UseWASI) {
    __wasi_filesize_t Pos;
    auto Result = Env->fdTell(Fd, Pos);
    if (!Result) {
      setError();
      return -1;
    }

    return static_cast<std::streampos>(static_cast<std::streamoff>(Pos));
  }
  std::streampos Pos = StdStream.tellp();
  if (StdStream.fail()) {
    setError();
    return -1;
  }
  return Pos;
}

OFStream &OFStream::seekp(std::streampos Pos) {
  if (!is_open()) {
    setError();
    return *this;
  }

  if (UseWASI) {
    __wasi_filesize_t NewPos;
    auto Result = Env->fdSeek(Fd, static_cast<__wasi_filedelta_t>(Pos),
                              __WASI_WHENCE_SET, NewPos);
    if (!Result) {
      setError();
    }
  } else {
    StdStream.seekp(Pos);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

OFStream &OFStream::seekp(std::streamoff Off, std::ios_base::seekdir Way) {
  if (!is_open()) {
    setError();
    return *this;
  }

  if (UseWASI) {
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
  } else {
    StdStream.seekp(Off, Way);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

void OFStream::close() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
    IsOpen = false;
  }
}

} // namespace FStream
} // namespace WasmEdge
