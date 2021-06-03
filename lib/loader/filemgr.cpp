// SPDX-License-Identifier: Apache-2.0
#include "loader/filemgr.h"
#include "common/filesystem.h"

#include <algorithm>
#include <iterator>

/// Error logging of file manager need to be handled in caller.

namespace WasmEdge {

/// Set path to file manager. See "include/loader/filemgr.h".
Expect<void> FileMgr::setPath(const std::filesystem::path &FilePath) {
  reset();
  std::error_code ErrCode;
  Size = std::filesystem::file_size(FilePath, ErrCode);
  if (likely(!ErrCode)) {
    if (!MMap::supported()) {
      Status = ErrCode::InvalidPath;
      return Unexpect(Status);
    }
    Map.emplace(FilePath);
    if (auto *Pointer = Map->address(); likely(Pointer)) {
      Data = reinterpret_cast<const Byte *>(Pointer);
      Status = ErrCode::Success;
      return {};
    }
    Map.reset();
  }
  Status = ErrCode::InvalidPath;
  return Unexpect(Status);
}

/// Set code data. See "include/loader/filemgr.h".
Expect<void> FileMgr::setCode(Span<const Byte> CodeData) {
  reset();
  Data = CodeData.data();
  Size = CodeData.size();
  if (Size == 0) {
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  Status = ErrCode::Success;
  return {};
}

/// Set code data. See "include/loader/filemgr.h".
Expect<void> FileMgr::setCode(std::vector<Byte> CodeData) {
  reset();
  DataHolder.emplace(std::move(CodeData));
  Data = DataHolder->data();
  Size = DataHolder->size();
  if (Size == 0) {
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  Status = ErrCode::Success;
  return {};
}

/// Read one byte. See "include/loader/filemgr.h".
Expect<Byte> FileMgr::readByte() {
  if (unlikely(Status != ErrCode::Success)) {
    return Unexpect(Status);
  }
  if (unlikely(Pos >= Size)) {
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  return Data[Pos++];
}

/// Read number of bytes. See "include/loader/filemgr.h".
Expect<std::vector<Byte>> FileMgr::readBytes(size_t SizeToRead) {
  std::vector<Byte> Buf(SizeToRead);
  if (auto Res = readBytes(Buf); unlikely(!Res)) {
    return Unexpect(Res);
  }
  return Buf;
}

/// Read number of bytes. See "include/loader/filemgr.h".
Expect<void> FileMgr::readBytes(Span<Byte> Buffer) {
  if (unlikely(Status != ErrCode::Success)) {
    return Unexpect(Status);
  }
  auto SizeToRead = Buffer.size();
  if (likely(SizeToRead > 0)) {
    if (Pos + SizeToRead > Size) {
      SizeToRead = Size - Pos;
      Pos = Size;
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    std::copy_n(Data + Pos, SizeToRead, Buffer.begin());
    Pos += SizeToRead;
  }
  return {};
}

/// Decode and read an unsigned int. See "include/loader/filemgr.h".
Expect<uint32_t> FileMgr::readU32() {
  if (unlikely(Status != ErrCode::Success)) {
    return Unexpect(Status);
  }
  uint32_t Result = 0;
  uint32_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 32)) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    Result |= (Byte & UINT32_C(0x7F)) << Offset;
    if (Offset == 28 && unlikely((Byte & UINT32_C(0x70)) != 0)) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

/// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Expect<uint64_t> FileMgr::readU64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  uint64_t Result = 0;
  uint64_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 64)) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    Result |= (Byte & UINT64_C(0x7F)) << Offset;
    if (Offset == 63 && unlikely((Byte & UINT32_C(0x7E)) != 0)) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

/// Decode and read a signed int. See "include/loader/filemgr.h".
Expect<int32_t> FileMgr::readS32() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  int32_t Result = 0;
  uint32_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 32)) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    const int32_t Mask = Offset == 28 ? INT32_C(0x0F) : INT32_C(0x7F);
    Result |= (Byte & Mask) << Offset;
    Offset += 7;
  }
  if (Offset == 35) {
    /// The signed-extend bits should be the same.
    if (unlikely(((Byte & 0x70) != 0x70 && (Byte & 0x70) != 0) ||
                 (Byte & 0x40) >> 6 != (Byte & 0x08) >> 3)) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 32) {
    Result |= static_cast<int32_t>(UINT32_C(0xFFFFFFFF) << Offset);
  }
  return Result;
}

/// Decode and read a signed long long int. See "include/loader/filemgr.h".
Expect<int64_t> FileMgr::readS64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  int64_t Result = 0;
  uint64_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 64)) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    const int64_t Mask = Offset == 63 ? INT64_C(0x01) : INT64_C(0x7F);
    Result |= (Byte & Mask) << Offset;
    Offset += 7;
  }
  if (Offset == 70) {
    /// The signed-extend bits should be the same.
    if (unlikely(((Byte & 0x7E) != 0x7E && (Byte & 0x7E) != 0) ||
                 (Byte & 0x40) >> 6 != (Byte & 0x01))) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 64) {
    Result |= static_cast<int64_t>(UINT64_C(0xFFFFFFFFFFFFFFFF) << Offset);
  }
  return Result;
}

/// Copy bytes to a float. See "include/loader/filemgr.h".
Expect<float> FileMgr::readF32() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  uint32_t Buf = 0;
  Byte Byte = 0x00;
  for (uint32_t I = 0; I < 4; I++) {
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    Buf |= (Byte & UINT32_C(0xFF)) << (I * UINT32_C(8));
  }
  float Result;
  static_assert(sizeof(Buf) == sizeof(Result));
  std::memcpy(&Result, &Buf, sizeof(Result));
  return Result;
}

/// Copy bytes to a double. See "include/loader/filemgr.h".
Expect<double> FileMgr::readF64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  uint64_t Buf = 0;
  Byte Byte = 0x00;
  for (uint32_t I = 0; I < 8; I++) {
    if (auto Res = readByte(); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = *Res;
    }
    Buf |= (Byte & UINT64_C(0xFF)) << (I * UINT64_C(8));
  }
  double Result;
  static_assert(sizeof(Buf) == sizeof(Result));
  std::memcpy(&Result, &Buf, sizeof(Result));
  return Result;
}

/// Read a vector of bytes. See "include/loader/filemgr.h".
Expect<std::string> FileMgr::readName() {
  if (unlikely(Status != ErrCode::Success)) {
    return Unexpect(Status);
  }
  uint32_t SizeToRead;
  if (auto Res = readU32(); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    SizeToRead = *Res;
  }
  std::string Str(SizeToRead, '\0');
  if (auto Res = readBytes(
          Span<Byte>(reinterpret_cast<Byte *>(Str.data()), Str.size()));
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  /// UTF-8 validation.
  bool Valid = true;
  for (uint32_t I = 0; I < Str.size() && Valid; ++I) {
    char C = Str.data()[I];
    uint32_t N = 0;
    if ((C & '\x80') == 0) {
      /// 0xxxxxxx, 7 bits UCS, ASCII
      N = 0;
    } else if ((C & '\xE0') == '\xC0') {
      /// 110xxxxx, 11 bits UCS, U+80 to U+7FF
      N = 1;
    } else if ((C & '\xF0') == '\xE0') {
      /// 1110xxxx, 16 bits UCS, U+800 to U+D7FF and U+E000 to U+FFFF
      N = 2;
    } else if ((C & '\xF8') == '\xF0') {
      /// 11110xxx, 21 bits UCS, U+10000 to U+10FFFF
      N = 3;
    } else {
      Valid = false;
    }

    /// Need to have N more bytes
    if (I + N >= Str.size()) {
      Valid = false;
    }
    /// Invalid ranges
    if (N == 1 && (C & '\xDE') == '\xC0') {
      /// 11 bits UCS, U+0 to U+80, FAIL
      Valid = false;
    } else if (N == 2 &&
               ((C == '\xE0' && (Str.data()[I + 1] & '\xA0') == '\x80') ||
                /// 16 bits UCS, U+0 to U+7FF, FAIL
                (C == '\xED' && (Str.data()[I + 1] & '\xA0') == '\xA0')
                /// 16 bits UCS, U+D800 to U+DFFF, FAIL
                )) {
      Valid = false;
    } else if (N == 3 &&
               ((C == '\xF0' && (Str.data()[I + 1] & '\xB0') == '\x80') ||
                /// 21 bits UCS, U+0 to U+FFFF, FAIL
                (C == '\xF4' && (Str.data()[I + 1] & '\xB0') != '\x80') ||
                /// 21 bits UCS, U+110000 to U+13FFFF, FAIL
                (C != '\xF4' && (C & '\xF4') == '\xF4')
                /// 21 bits UCS, U+140000 to U+1FFFFF, FAIL
                )) {
      Valid = false;
    }

    for (uint32_t J = 0; J < N && Valid; ++J) {
      /// N bytes needs to match 10xxxxxx
      if ((Str.data()[I + J + 1] & '\xC0') != '\x80') {
        Valid = false;
      }
    }
    I += N;
  }

  if (!Valid) {
    Status = ErrCode::InvalidUTF8;
    return Unexpect(Status);
  }
  return Str;
}

} // namespace WasmEdge
