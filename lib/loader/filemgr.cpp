// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/filemgr.h"

#include <algorithm>
#include <iterator>

// Error logging of file manager need to be handled in caller.

namespace WasmEdge {

// Set path to file manager. See "include/loader/filemgr.h".
Expect<void> FileMgr::setPath(const std::filesystem::path &FilePath) {
  reset();
  std::error_code ErrCode;
  Size = std::filesystem::file_size(FilePath, ErrCode);
  if (likely(!ErrCode)) {
    if (!MMap::supported()) {
      Size = 0;
      Status = ErrCode::Value::IllegalPath;
      return Unexpect(Status);
    }
    FileMap.emplace(FilePath);
    if (auto *Pointer = FileMap->address(); likely(Pointer)) {
      Data = reinterpret_cast<const Byte *>(Pointer);
      Status = ErrCode::Value::Success;
    } else {
      // File size is 0, mmap failed.
      // Will get 'UnexpectedEnd' error while the first reading.
      FileMap.reset();
    }
    return {};
  }
  Size = 0;
  Status = ErrCode::Value::IllegalPath;
  return Unexpect(Status);
}

// Set code data. See "include/loader/filemgr.h".
Expect<void> FileMgr::setCode(Span<const Byte> CodeData) {
  reset();
  Data = CodeData.data();
  Size = CodeData.size();
  Status = ErrCode::Value::Success;
  return {};
}

// Set code data. See "include/loader/filemgr.h".
Expect<void> FileMgr::setCode(std::vector<Byte> CodeData) {
  reset();
  // Tell GCC 14 that DataHolder has no data.
  // Fix the false positive warning,
  // which is reported by GCC 14 with `maybe-uninitialized`
  assuming(!DataHolder);

  DataHolder.emplace(std::move(CodeData));
  Data = DataHolder->data();
  Size = DataHolder->size();
  Status = ErrCode::Value::Success;
  return {};
}

// Read one byte. See "include/loader/filemgr.h".
Expect<Byte> FileMgr::readByte() {
  if (unlikely(Status != ErrCode::Value::Success)) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;
  // Check if exceed the data boundary and section boundary.
  if (auto Res = testRead(1); unlikely(!Res)) {
    return Unexpect(Res);
  }
  return Data[Pos++];
}

// Read number of bytes. See "include/loader/filemgr.h".
Expect<std::vector<Byte>> FileMgr::readBytes(size_t SizeToRead) {
  // Set the flag to the start offset.
  LastPos = Pos;
  // Read bytes into vector.
  std::vector<Byte> Buf(SizeToRead);
  if (auto Res = readBytes(Buf); unlikely(!Res)) {
    return Unexpect(Res);
  }
  return Buf;
}

// Decode and read an unsigned int. See "include/loader/filemgr.h".
Expect<uint32_t> FileMgr::readU32() {
  if (unlikely(Status != ErrCode::Value::Success)) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;

  // Read and decode U32.
  uint32_t Result = 0;
  uint32_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 32)) {
      Status = ErrCode::Value::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = testRead(1); unlikely(!Res)) {
      return Unexpect(Res);
    }
    Byte = Data[Pos++];
    Result |= (Byte & UINT32_C(0x7F)) << Offset;
    if (Offset == 28 && unlikely((Byte & UINT32_C(0x70)) != 0)) {
      Status = ErrCode::Value::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Expect<uint64_t> FileMgr::readU64() {
  if (Status != ErrCode::Value::Success) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;

  // Read and decode U64.
  uint64_t Result = 0;
  uint64_t Offset = 0;
  Byte Byte = 0x80;
  while (Byte & 0x80) {
    if (unlikely(Offset >= 64)) {
      Status = ErrCode::Value::IntegerTooLong;
      return Unexpect(Status);
    }
    if (auto Res = testRead(1); unlikely(!Res)) {
      return Unexpect(Res);
    }
    Byte = Data[Pos++];
    Result |= (Byte & UINT64_C(0x7F)) << Offset;
    if (Offset == 63 && unlikely((Byte & UINT32_C(0x7E)) != 0)) {
      Status = ErrCode::Value::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

template <typename RetType, size_t N> Expect<RetType> FileMgr::readSN() {
  static_assert(N >= 8, "The N of S_N must have at least length of a byte");
  static_assert(8 * sizeof(RetType) >= N,
                "RetType cannot hold the full range of S_N");
  static_assert(std::is_signed_v<RetType>,
                "RetType of S_N must be signed type");

  if (Status != ErrCode::Value::Success) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;

  // Read and decode S_N.
  RetType Result = 0;
  size_t Offset = 0;
  size_t RemainingBits = N;
  while (true) {
    if (RemainingBits <= 0) {
      Status = ErrCode::Value::IntegerTooLong;
      return Unexpect(Status);
    }

    // In the rest logic, RemainingBits must be at least 1.
    WasmEdge::Byte Byte;
    if (auto Res = testRead(1); unlikely(!Res)) {
      return Unexpect(Res);
    } else {
      Byte = Data[Pos++];
    }

    const WasmEdge::Byte HighestBitMask = 1 << 7;
    const WasmEdge::Byte SecondHighestBitMask = 1 << 6;
    if (Byte & HighestBitMask) {
      // The byte has leading 1. It contains 7 bits payload.

      if (unlikely(RemainingBits < 7)) {
        Status = ErrCode::Value::IntegerTooLong;
        return Unexpect(Status);
      }

      std::make_unsigned_t<RetType> Payload =
          Byte & (~HighestBitMask); // & 0b01111111
      Result |= (Payload << Offset);
      Offset += 7;
      RemainingBits -= 7;
    } else {
      // The byte has leading 0. It will be the last byte.

      // The number of bits that take effect in the byte. Since RemainingBits
      // must be at least 1, EffectiveBits also must be at least 1. It is also
      // at most 7.
      size_t EffectiveBits = RemainingBits < 7 ? RemainingBits : 7;
      std::make_unsigned_t<RetType> Payload = Byte;
      if (Byte & SecondHighestBitMask) {
        // The byte is signed.
        if (Byte >= (1 << 7) - (1 << (EffectiveBits - 1))) {
          Payload -= (1 << 7);
        } else {
          Status = ErrCode::Value::IntegerTooLarge;
          return Unexpect(Status);
        }
      } else {
        // The byte is unsigned.
        if (Byte >= (1 << (EffectiveBits - 1))) {
          Status = ErrCode::Value::IntegerTooLarge;
          return Unexpect(Status);
        }
      }
      Result |= (Payload << Offset);
      break;
    }
  }

  return Result;
}

Expect<int64_t> FileMgr::readS33() { return readSN<int64_t, 33>(); }

// Decode and read a signed int. See "include/loader/filemgr.h".
Expect<int32_t> FileMgr::readS32() { return readSN<int32_t, 32>(); }

// Decode and read a signed long long int. See "include/loader/filemgr.h".
Expect<int64_t> FileMgr::readS64() { return readSN<int64_t, 64>(); }

// Copy bytes to a float. See "include/loader/filemgr.h".
Expect<float> FileMgr::readF32() {
  if (Status != ErrCode::Value::Success) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;

  uint32_t Buf = 0;
  Byte Byte = 0x00;
  // Check if exceed the data boundary and section boundary.
  if (auto Res = testRead(4); unlikely(!Res)) {
    return Unexpect(Res);
  }
  for (uint32_t I = 0; I < 4; I++) {
    Byte = Data[Pos++];
    Buf |= (Byte & UINT32_C(0xFF)) << (I * UINT32_C(8));
  }
  float Result;
  static_assert(sizeof(Buf) == sizeof(Result));
  std::memcpy(&Result, &Buf, sizeof(Result));
  return Result;
}

// Copy bytes to a double. See "include/loader/filemgr.h".
Expect<double> FileMgr::readF64() {
  if (Status != ErrCode::Value::Success) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;

  uint64_t Buf = 0;
  Byte Byte = 0x00;
  // Check if exceed the data boundary and section boundary.
  if (auto Res = testRead(8); unlikely(!Res)) {
    return Unexpect(Res);
  }
  for (uint32_t I = 0; I < 8; I++) {
    Byte = Data[Pos++];
    Buf |= (Byte & UINT64_C(0xFF)) << (I * UINT64_C(8));
  }
  double Result;
  static_assert(sizeof(Buf) == sizeof(Result));
  std::memcpy(&Result, &Buf, sizeof(Result));
  return Result;
}

// Read a vector of bytes. See "include/loader/filemgr.h".
Expect<std::string> FileMgr::readName() {
  if (unlikely(Status != ErrCode::Value::Success)) {
    return Unexpect(Status);
  }
  // If UTF-8 validation or readU32() or readBytes() failed, the last succeeded
  // reading offset will be at the start of `Name`.
  LastPos = Pos;

  // Read the name size.
  uint32_t SizeToRead;
  if (auto Res = readU32(); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    SizeToRead = *Res;
  }

  // Check if string length exceed the data boundary.
  if (auto Res = testRead(SizeToRead); unlikely(!Res)) {
    return Unexpect(ErrCode::Value::LengthOutOfBounds);
  }

  // Read the UTF-8 bytes.
  std::string Str(SizeToRead, '\0');
  if (auto Res = readBytes(
          Span<Byte>(reinterpret_cast<Byte *>(Str.data()), Str.size()));
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  // UTF-8 validation.
  bool Valid = true;
  for (uint32_t I = 0; I < Str.size() && Valid; ++I) {
    char C = Str.data()[I];
    uint32_t N = 0;
    if ((C & '\x80') == 0) {
      // 0xxxxxxx, 7 bits UCS, ASCII
      N = 0;
    } else if ((C & '\xE0') == '\xC0') {
      // 110xxxxx, 11 bits UCS, U+80 to U+7FF
      N = 1;
    } else if ((C & '\xF0') == '\xE0') {
      // 1110xxxx, 16 bits UCS, U+800 to U+D7FF and U+E000 to U+FFFF
      N = 2;
    } else if ((C & '\xF8') == '\xF0') {
      // 11110xxx, 21 bits UCS, U+10000 to U+10FFFF
      N = 3;
    } else {
      Valid = false;
    }

    // Need to have N more bytes
    if (I + N >= Str.size()) {
      Valid = false;
    }
    // Invalid ranges
    if (N == 1 && (C & '\xDE') == '\xC0') {
      // 11 bits UCS, U+0 to U+80, FAIL
      Valid = false;
    } else if (N == 2 &&
               ((C == '\xE0' && (Str.data()[I + 1] & '\xA0') == '\x80') ||
                // 16 bits UCS, U+0 to U+7FF, FAIL
                (C == '\xED' && (Str.data()[I + 1] & '\xA0') == '\xA0')
                // 16 bits UCS, U+D800 to U+DFFF, FAIL
                )) {
      Valid = false;
    } else if (N == 3 &&
               ((C == '\xF0' && (Str.data()[I + 1] & '\xB0') == '\x80') ||
                // 21 bits UCS, U+0 to U+FFFF, FAIL
                (C == '\xF4' && (Str.data()[I + 1] & '\xB0') != '\x80') ||
                // 21 bits UCS, U+110000 to U+13FFFF, FAIL
                (C != '\xF4' && (C & '\xF4') == '\xF4')
                // 21 bits UCS, U+140000 to U+1FFFFF, FAIL
                )) {
      Valid = false;
    }

    for (uint32_t J = 0; J < N && Valid; ++J) {
      // N bytes needs to match 10xxxxxx
      if ((Str.data()[I + J + 1] & '\xC0') != '\x80') {
        Valid = false;
      }
    }
    I += N;
  }

  if (!Valid) {
    Status = ErrCode::Value::MalformedUTF8;
    return Unexpect(Status);
  }
  return Str;
}

// Peek one byte. See "include/loader/filemgr.h".
Expect<Byte> FileMgr::peekByte() {
  if (auto Res = readByte()) {
    Pos--;
    return Res;
  } else {
    return Unexpect(Res);
  }
}

// Get the file header type. See "include/loader/filemgr.h".
FileMgr::FileHeader FileMgr::getHeaderType() {
  if (Size >= 4) {
    Byte WASMMagic[] = {0x00, 0x61, 0x73, 0x6D};
    Byte ELFMagic[] = {0x7F, 0x45, 0x4C, 0x46};
    Byte MAC32agic[] = {0xCE, 0xFA, 0xED, 0xFE};
    Byte MAC64agic[] = {0xCF, 0xFA, 0xED, 0xFE};
    if (std::equal(WASMMagic, WASMMagic + 4, Data)) {
      return FileMgr::FileHeader::Wasm;
    } else if (std::equal(ELFMagic, ELFMagic + 4, Data)) {
      return FileMgr::FileHeader::ELF;
    } else if (std::equal(MAC32agic, MAC32agic + 4, Data)) {
      return FileMgr::FileHeader::MachO_32;
    } else if (std::equal(MAC64agic, MAC64agic + 4, Data)) {
      return FileMgr::FileHeader::MachO_64;
    }
  }
  if (Size >= 2) {
    Byte DLLMagic[] = {0x4D, 0x5A};
    if (std::equal(DLLMagic, DLLMagic + 2, Data)) {
      return FileMgr::FileHeader::DLL;
    }
  }
  return FileMgr::FileHeader::Unknown;
}

// Jump a section. See "include/loader/filemgr.h".
Expect<void> FileMgr::jumpContent() {
  if (unlikely(Status != ErrCode::Value::Success)) {
    return Unexpect(Status);
  }
  // Set the flag to the start offset.
  LastPos = Pos;
  // Read the section size.
  uint32_t SecSize = 0;
  if (auto Res = readU32()) {
    SecSize = *Res;
  } else {
    return Unexpect(Res);
  }
  // Jump the content.
  if (auto Res = testRead(SecSize); unlikely(!Res)) {
    return Unexpect(ErrCode::Value::LengthOutOfBounds);
  }
  Pos += SecSize;
  return {};
}

// Helper function for reading number of bytes. See "include/loader/filemgr.h".
Expect<void> FileMgr::readBytes(Span<Byte> Buffer) {
  if (unlikely(Status != ErrCode::Value::Success)) {
    return Unexpect(Status);
  }
  // The adjustment of `LastPos` should be handled by caller.
  auto SizeToRead = Buffer.size();
  if (likely(SizeToRead > 0)) {
    // Check if exceed the data boundary.
    if (auto Res = testRead(SizeToRead); unlikely(!Res)) {
      return Unexpect(Res);
    }
    std::copy_n(Data + Pos, SizeToRead, Buffer.begin());
    Pos += SizeToRead;
  }
  return {};
}

// Helper function for checking boundary. See "include/loader/filemgr.h".
Expect<void> FileMgr::testRead(uint64_t Read) {
  // Check if exceed the data boundary
  if (unlikely(getRemainSize() < Read)) {
    Pos = Size;
    LastPos = Pos;
    Status = ErrCode::Value::UnexpectedEnd;
    return Unexpect(Status);
  }
  return {};
}

} // namespace WasmEdge
