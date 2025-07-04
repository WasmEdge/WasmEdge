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
    : Fd(0), IsOpen(false), HasError(false), IsEof(false),
      UseWASI(WASIEnv != nullptr) {

  if (UseWASI) {
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
      spdlog::error("Failed to open file for reading: {}",
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

WasmEdgeIfstream::~WasmEdgeIfstream() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
  }
}

bool WasmEdgeIfstream::is_open() const noexcept {
  if (UseWASI) {
    return IsOpen;
  } else {
    return StdStream.is_open();
  }
}

bool WasmEdgeIfstream::good() const noexcept {
  if (UseWASI) {
    return !HasError;
  } else {
    return StdStream.good();
  }
}

bool WasmEdgeIfstream::eof() const noexcept {
  if (UseWASI) {
    return IsEof;
  } else {
    return StdStream.eof();
  }
}

bool WasmEdgeIfstream::fail() const noexcept {
  if (UseWASI) {
    return HasError;
  } else {
    return StdStream.fail();
  }
}

WasmEdgeIfstream &WasmEdgeIfstream::read(char *Buffer, std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  if (UseWASI) {
    Span<uint8_t> BufferSpan(reinterpret_cast<uint8_t *>(Buffer), Count);
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

std::streamsize WasmEdgeIfstream::readsome(char *Buffer,
                                           std::streamsize Count) {
  if (!good() || Count <= 0) {
    return 0;
  }

  if (UseWASI) {
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
  } else {
    std::streamsize Result = StdStream.readsome(Buffer, Count);
    if (StdStream.fail()) {
      setError();
      return 0;
    }
    return Result;
  }
}

int WasmEdgeIfstream::get() {
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
  } else {
    int C = StdStream.get();
    if (StdStream.fail() && !StdStream.eof()) {
      setError();
    }
    return C;
  }
}

WasmEdgeIfstream &WasmEdgeIfstream::getline(std::string &Line, char Delim) {
  Line.clear();

  if (UseWASI) {
    int C;
    while ((C = get()) != EOF && C != Delim) {
      Line += static_cast<char>(C);
    }

    if (C == EOF && Line.empty()) {
      setError();
    }
  } else {
    std::getline(StdStream, Line, Delim);
    if (StdStream.fail() && !StdStream.eof()) {
      setError();
    }
  }

  return *this;
}

std::string WasmEdgeIfstream::getline(char Delim) {
  std::string Line;
  getline(Line, Delim);
  return Line;
}

std::streampos WasmEdgeIfstream::tellg() {
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

    return static_cast<std::streampos>(Pos);
  } else {
    std::streampos Pos = StdStream.tellg();
    if (StdStream.fail()) {
      setError();
      return -1;
    }
    return Pos;
  }
}

WasmEdgeIfstream &WasmEdgeIfstream::seekg(std::streampos Pos) {
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

WasmEdgeIfstream &WasmEdgeIfstream::seekg(std::streamoff Off,
                                          std::ios_base::seekdir Way) {
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

WasmEdgeOfstream::WasmEdgeOfstream(const Host::WASI::Environ *WASIEnv,
                                   const std::string_view &FileName) noexcept
    : Fd(0), IsOpen(false), HasError(false), ChunkSize(64 * 1024),
      UseWASI(WASIEnv != nullptr) {

  if (UseWASI) {
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
      spdlog::error("Failed to open file for writing: {}",
                    std::string(FileName));
    }
  } else {
    StdStream.open(std::string(FileName), std::ios::out | std::ios::trunc);
    IsOpen = StdStream.is_open();
    if (!IsOpen) {
      HasError = true;
    }
  }
}

WasmEdgeOfstream::~WasmEdgeOfstream() {
  if (IsOpen) {
    if (UseWASI) {
      Env->fdClose(Fd);
    } else {
      StdStream.close();
    }
  }
}

bool WasmEdgeOfstream::is_open() const noexcept {
  if (UseWASI) {
    return IsOpen;
  } else {
    return StdStream.is_open();
  }
}

bool WasmEdgeOfstream::good() const noexcept {
  if (UseWASI) {
    return !HasError;
  } else {
    return StdStream.good();
  }
}

bool WasmEdgeOfstream::fail() const noexcept {
  if (UseWASI) {
    return HasError;
  } else {
    return StdStream.fail();
  }
}

WasmEdgeOfstream &WasmEdgeOfstream::write(const char *Buffer,
                                          std::streamsize Count) {
  if (!good() || Count <= 0) {
    return *this;
  }

  if (UseWASI) {
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
  } else {
    StdStream.write(Buffer, Count);
    if (StdStream.fail()) {
      setError();
    }
  }

  return *this;
}

WasmEdgeOfstream &WasmEdgeOfstream::put(char C) { return write(&C, 1); }

WasmEdgeOfstream &WasmEdgeOfstream::flush() {
  if (UseWASI) {
    // WASI 直接写入文件，无需缓冲区刷新
  } else {
    StdStream.flush();
    if (StdStream.fail()) {
      setError();
    }
  }
  return *this;
}

std::streampos WasmEdgeOfstream::tellp() {
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

    return static_cast<std::streampos>(Pos);
  } else {
    std::streampos Pos = StdStream.tellp();
    if (StdStream.fail()) {
      setError();
      return -1;
    }
    return Pos;
  }
}

WasmEdgeOfstream &WasmEdgeOfstream::seekp(std::streampos Pos) {
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

WasmEdgeOfstream &WasmEdgeOfstream::seekp(std::streamoff Off,
                                          std::ios_base::seekdir Way) {
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

} // namespace API
} // namespace Host
} // namespace WasmEdge