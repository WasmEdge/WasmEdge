// SPDX-License-Identifier: Apache-2.0
#include "loader/filemgr.h"
#include "common/filesystem.h"

#include <algorithm>
#include <iterator>

/// Error logging of file manager need to be handled in caller.

namespace SSVM {

/// Destructor of file manager. See "include/loader/filemgr.h".
FileMgrFStream::~FileMgrFStream() noexcept {
  if (Fin.is_open()) {
    Fin.close();
  }
}

/// Set path to file manager. See "include/loader/filemgr.h".
Expect<void> FileMgrFStream::setPath(const std::filesystem::path &FilePath) {
  if (Fin.is_open()) {
    Fin.close();
    Status = ErrCode::InvalidPath;
  }
  Fin.open(FilePath, std::ios::in | std::ios::binary);
  if (!Fin.fail()) {
    Status = ErrCode::Success;
    FSize = std::filesystem::file_size(FilePath);
  }
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  return {};
}

/// Read one byte. See "include/loader/filemgr.h".
Expect<Byte> FileMgrFStream::readByte() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  char Buf = 0;
  Fin.get(Buf);
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  return static_cast<unsigned char>(Buf);
}

/// Read number of bytes. See "include/loader/filemgr.h".
Expect<std::vector<Byte>> FileMgrFStream::readBytes(size_t SizeToRead) {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  std::vector<Byte> Buf;
  Buf.resize(SizeToRead);
  if (SizeToRead > 0) {
    Fin.read(reinterpret_cast<char *>(&Buf[0]), SizeToRead);
  }
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  return Buf;
}

/// Decode and read an unsigned int. See "include/loader/filemgr.h".
Expect<uint32_t> FileMgrFStream::readU32() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  uint32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    if (Offset >= 32) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    Fin.get(Byte);
    Result |= (Byte & UINT32_C(0x7F)) << (Offset);
    if (Offset == 28 && (Byte & UINT32_C(0x70)) != 0) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  return Result;
}

/// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Expect<uint64_t> FileMgrFStream::readU64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  uint64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    if (Offset >= 64) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    Fin.get(Byte);
    Result |= (Byte & UINT64_C(0x7F)) << (Offset);
    if (Offset == 63 && (Byte & UINT32_C(0x7E)) != 0) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  return Result;
}

/// Decode and read a signed int. See "include/loader/filemgr.h".
Expect<int32_t> FileMgrFStream::readS32() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  int32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    if (Offset >= 32) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    Fin.get(Byte);
    Result |= (Byte & UINT32_C(0x7F)) << (Offset);
    Offset += 7;
  }
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  if (Offset == 35) {
    /// The signed-extend bits should be the same.
    if (((Byte & 0x70) != 0x70 && (Byte & 0x70) != 0) ||
        (Byte & 0x40) >> 6 != (Byte & 0x08) >> 3) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 32) {
    Result |= 0xFFFFFFFF << Offset;
  }
  return Result;
}

/// Decode and read a signed long long int. See "include/loader/filemgr.h".
Expect<int64_t> FileMgrFStream::readS64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  int64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    if (Offset >= 64) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    Fin.get(Byte);
    Result |= (Byte & UINT64_C(0x7F)) << (Offset);
    Offset += 7;
  }
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  if (Offset == 70) {
    /// The signed-extend bits should be the same.
    if (((Byte & 0x7E) != 0x7E && (Byte & 0x7E) != 0) ||
        (Byte & 0x40) >> 6 != (Byte & 0x01)) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 64) {
    Result |= 0xFFFFFFFFFFFFFFFFULL << Offset;
  }
  return Result;
}

/// Copy bytes to a float. See "include/loader/filemgr.h".
Expect<float> FileMgrFStream::readF32() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  union {
    uint32_t U;
    float F;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 4; i++) {
    Fin.get(Byte);
    if (Fin.fail()) {
      Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
      return Unexpect(Status);
    }
    Val.U |= (Byte & 0xFF) << (i * 8);
  }
  return Val.F;
}

/// Copy bytes to a double. See "include/loader/filemgr.h".
Expect<double> FileMgrFStream::readF64() {
  if (Status != ErrCode::Success) {
    return Unexpect(Status);
  }
  union {
    uint64_t U;
    double D;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 8; i++) {
    Fin.get(Byte);
    if (Fin.fail()) {
      Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
      return Unexpect(Status);
    }
    Val.U |= static_cast<uint64_t>(Byte & 0xFF) << (i * 8);
  }
  return Val.D;
}

/// Read a vector of bytes. See "include/loader/filemgr.h".
Expect<std::string> FileMgrFStream::readName() {
  Expect<uint32_t> Size = readU32();
  if (!Size) {
    return Unexpect(Size);
  }
  std::string Str(*Size, '\0');
  Fin.read(&Str[0], *Size);
  if (Fin.fail()) {
    Status = Fin.eof() ? ErrCode::EndOfFile : ErrCode::ReadError;
    return Unexpect(Status);
  }
  return Str;
}

/// Set code data. See "include/loader/filemgr.h".
Expect<void> FileMgrVector::setCode(Span<const Byte> CodeData) {
  Code.assign(CodeData.begin(), CodeData.end());
  Pos = 0;
  if (Code.size() == 0) {
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  Status = ErrCode::Success;
  return {};
}

/// Read one byte. See "include/loader/filemgr.h".
Expect<Byte> FileMgrVector::readByte() {
  if (Pos >= Code.size()) {
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  return Code[Pos++];
}

/// Read number of bytes. See "include/loader/filemgr.h".
Expect<std::vector<Byte>> FileMgrVector::readBytes(size_t SizeToRead) {
  std::vector<Byte> Buf;
  Buf.resize(SizeToRead);
  if (SizeToRead > 0) {
    if (Pos + SizeToRead > Code.size()) {
      Pos = Code.size();
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    std::copy_n(Code.begin() + Pos, SizeToRead, Buf.begin());
    Pos += SizeToRead;
  }
  return Buf;
}

/// Decode and read an unsigned int. See "include/loader/filemgr.h".
Expect<uint32_t> FileMgrVector::readU32() {
  uint32_t Result = 0;
  uint32_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Offset >= 32) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (Pos >= Code.size()) {
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    Byte = Code[Pos++];
    Result |= (Byte & UINT32_C(0x7F)) << (Offset);
    if (Offset == 28 && (Byte & UINT32_C(0x70)) != 0) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

/// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Expect<uint64_t> FileMgrVector::readU64() {
  uint64_t Result = 0;
  uint64_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Offset >= 64) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (Pos >= Code.size()) {
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    Byte = Code[Pos++];
    Result |= (Byte & UINT64_C(0x7F)) << (Offset);
    if (Offset == 63 && (Byte & UINT32_C(0x7E)) != 0) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
    Offset += 7;
  }
  return Result;
}

/// Decode and read a signed int. See "include/loader/filemgr.h".
Expect<int32_t> FileMgrVector::readS32() {
  int32_t Result = 0;
  uint32_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Offset >= 32) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (Pos >= Code.size()) {
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    Byte = Code[Pos++];
    Result |= (Byte & UINT32_C(0x7F)) << Offset;
    Offset += 7;
  }
  if (Offset == 35) {
    /// The signed-extend bits should be the same.
    if (((Byte & 0x70) != 0x70 && (Byte & 0x70) != 0) ||
        (Byte & 0x40) >> 6 != (Byte & 0x08) >> 3) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 32) {
    Result |= 0xFFFFFFFF << Offset;
  }
  return Result;
}

/// Decode and read a signed long long int. See "include/loader/filemgr.h".
Expect<int64_t> FileMgrVector::readS64() {
  int64_t Result = 0;
  uint64_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Offset >= 64) {
      Status = ErrCode::IntegerTooLong;
      return Unexpect(Status);
    }
    if (Pos >= Code.size()) {
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    Byte = Code[Pos++];
    Result |= (Byte & UINT64_C(0x7F)) << Offset;
    Offset += 7;
  }
  if (Offset == 70) {
    /// The signed-extend bits should be the same.
    if (((Byte & 0x7E) != 0x7E && (Byte & 0x7E) != 0) ||
        (Byte & 0x40) >> 6 != (Byte & 0x01)) {
      Status = ErrCode::IntegerTooLarge;
      return Unexpect(Status);
    }
  }
  if (Byte & 0x40 && Offset < 64) {
    Result |= 0xFFFFFFFFFFFFFFFFULL << Offset;
  }
  return Result;
}

/// Copy bytes to a float. See "include/loader/filemgr.h".
Expect<float> FileMgrVector::readF32() {
  if (Pos + 4 > Code.size()) {
    Pos = Code.size();
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  union {
    uint32_t U;
    float F;
  } Val;
  Val.U = 0;
  for (int i = 0; i < 4; i++) {
    Val.U |= (Code[Pos++] & 0xFF) << (i * 8);
  }
  return Val.F;
}

/// Copy bytes to a double. See "include/loader/filemgr.h".
Expect<double> FileMgrVector::readF64() {
  if (Pos + 8 > Code.size()) {
    Pos = Code.size();
    Status = ErrCode::EndOfFile;
    return Unexpect(Status);
  }
  union {
    uint64_t U;
    double D;
  } Val;
  Val.U = 0;
  for (int i = 0; i < 8; i++) {
    Val.U |= static_cast<uint64_t>(Code[Pos++] & 0xFF) << (i * 8);
  }
  return Val.D;
}

/// Read a vector of bytes. See "include/loader/filemgr.h".
Expect<std::string> FileMgrVector::readName() {
  Expect<uint32_t> Size = readU32();
  if (!Size) {
    return Unexpect(Size);
  }
  std::string Str(*Size, '\0');
  if (*Size > 0) {
    if (Pos + *Size > Code.size()) {
      Pos = Code.size();
      Status = ErrCode::EndOfFile;
      return Unexpect(Status);
    }
    std::copy_n(Code.begin() + Pos, *Size, Str.begin());
    Pos += *Size;
  }
  return Str;
}

} // namespace SSVM
