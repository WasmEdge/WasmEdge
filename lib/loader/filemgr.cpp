// SPDX-License-Identifier: Apache-2.0
#include "loader/filemgr.h"

#include <algorithm>
#include <iterator>

namespace SSVM {

/// Destructor of file manager. See "include/loader/filemgr.h".
FileMgrFStream::~FileMgrFStream() {
  if (Fin.is_open()) {
    Fin.close();
  }
}

/// Set path to file manager. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::setPath(const std::string &FilePath) {
  if (Fin.is_open()) {
    Fin.close();
    Status = Loader::ErrCode::InvalidPath;
  }
  Path = FilePath;
  Fin.open(Path, std::ios::in | std::ios::binary);
  if (!Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  return Status;
}

/// Read one byte. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readByte(unsigned char &Byte) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  char Buf = 0;
  Fin.get(Buf);
  if (Fin.fail()) {
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
    return Status;
  }
  Byte = static_cast<unsigned char>(Buf);
  return Status;
}

/// Read number of bytes. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readBytes(std::vector<unsigned char> &Buf,
                                          size_t SizeToRead) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  if (SizeToRead > 0) {
    std::istreambuf_iterator<char> Iter(Fin);
    // TODO: error handling
    std::copy_n(Iter, SizeToRead, std::back_inserter(Buf));
    Iter++;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  return Status;
}

/// Decode and read an unsigned int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readU32(uint32_t &U32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  uint32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  else
    U32 = Result;
  return Status;
}

/// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readU64(uint64_t &U64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  uint64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= static_cast<uint64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  else
    U64 = Result;
  return Status;
}

/// Decode and read a signed int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readS32(int32_t &S32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  int32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  else {
    if (Byte & 0x40 && Offset < 32) {
      Result |= 0xFFFFFFFF << Offset;
    }
    S32 = Result;
  }
  return Status;
}

/// Decode and read a signed long long int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readS64(int64_t &S64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  int64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= static_cast<int64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  else {
    if (Byte & 0x40 && Offset < 64) {
      Result |= 0xFFFFFFFFFFFFFFFFULL << Offset;
    }
    S64 = Result;
  }
  return Status;
}

/// Copy bytes to a float. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readF32(float &F32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  union {
    uint32_t U;
    float F;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 4; i++) {
    Fin.get(Byte);
    if (Fin.fail()) {
      Status =
          Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
      return Status;
    }
    Val.U |= (Byte & 0xFF) << (i * 8);
  }
  F32 = Val.F;
  return Status;
}

/// Copy bytes to a double. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readF64(double &F64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  union {
    uint64_t U;
    double D;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 8; i++) {
    Fin.get(Byte);
    if (Fin.fail()) {
      Status =
          Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
      return Status;
    }
    Val.U |= static_cast<uint64_t>(Byte & 0xFF) << (i * 8);
  }
  F64 = Val.D;
  return Status;
}

/// Read a vector of bytes. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrFStream::readName(std::string &Str) {
  unsigned int Size = 0;
  if (readU32(Size) != Loader::ErrCode::Success)
    return Status;
  if (Size > 0) {
    std::istreambuf_iterator<char> Iter(Fin);
    std::copy_n(Iter, Size, std::back_inserter(Str));
    Iter++;
  }
  if (Fin.fail())
    Status = Fin.eof() ? Loader::ErrCode::EndOfFile : Loader::ErrCode::Success;
  return Status;
}

/// Set code data. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::setCode(const std::vector<uint8_t> &CodeData) {
  Code = CodeData;
  Pos = 0;
  Status =
      Code.size() > 0 ? Loader::ErrCode::Success : Loader::ErrCode::EndOfFile;
  return Status;
}

/// Read one byte. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readByte(unsigned char &Byte) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  if (Pos >= Code.size()) {
    Status = Loader::ErrCode::EndOfFile;
    return Status;
  }

  Byte = Code[Pos];
  Pos++;
  return Status;
}

/// Read number of bytes. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readBytes(std::vector<unsigned char> &Buf,
                                         size_t SizeToRead) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  if (SizeToRead > 0) {
    if (Pos + SizeToRead > Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    std::copy_n(Code.begin() + Pos, SizeToRead, std::back_inserter(Buf));
    Pos += SizeToRead;
  }
  return Status;
}

/// Decode and read an unsigned int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readU32(uint32_t &U32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  uint32_t Result = 0;
  uint32_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Pos >= Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    Byte = Code[Pos];
    Pos++;
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  U32 = Result;
  return Status;
}

/// Decode and read an unsigned long long int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readU64(uint64_t &U64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  uint64_t Result = 0;
  uint64_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Pos >= Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    Byte = Code[Pos];
    Pos++;
    Result |= static_cast<uint64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  U64 = Result;
  return Status;
}

/// Decode and read a signed int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readS32(int32_t &S32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  int32_t Result = 0;
  uint32_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Pos >= Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    Byte = Code[Pos];
    Pos++;
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Byte & 0x40 && Offset < 32) {
    Result |= 0xFFFFFFFF << Offset;
  }
  S32 = Result;
  return Status;
}

/// Decode and read a signed long long int. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readS64(int64_t &S64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  int64_t Result = 0;
  uint64_t Offset = 0;
  uint8_t Byte = 0x80;
  while (Byte & 0x80) {
    if (Pos >= Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    Byte = Code[Pos];
    Pos++;
    Result |= static_cast<int64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Byte & 0x40 && Offset < 64) {
    Result |= 0xFFFFFFFFFFFFFFFFULL << Offset;
  }
  S64 = Result;
  return Status;
}

/// Copy bytes to a float. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readF32(float &F32) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  if (Pos + 4 > Code.size()) {
    Status = Loader::ErrCode::EndOfFile;
    return Status;
  }
  union {
    uint32_t U;
    float F;
  } Val;
  Val.U = 0;
  for (int i = 0; i < 4; i++) {
    Val.U |= (Code[Pos] & 0xFF) << (i * 8);
    Pos++;
  }
  F32 = Val.F;
  return Status;
}

/// Copy bytes to a double. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readF64(double &F64) {
  if (Status != Loader::ErrCode::Success)
    return Status;
  if (Pos + 8 > Code.size()) {
    Status = Loader::ErrCode::EndOfFile;
    return Status;
  }
  union {
    uint64_t U;
    double D;
  } Val;
  Val.U = 0;
  for (int i = 0; i < 8; i++) {
    Val.U |= static_cast<uint64_t>(Code[Pos] & 0xFF) << (i * 8);
    Pos++;
  }
  F64 = Val.D;
  return Status;
}

/// Read a vector of bytes. See "include/loader/filemgr.h".
Loader::ErrCode FileMgrVector::readName(std::string &Str) {
  unsigned int Size = 0;
  if (readU32(Size) != Loader::ErrCode::Success)
    return Status;
  if (Size > 0) {
    if (Pos + Size > Code.size()) {
      Status = Loader::ErrCode::EndOfFile;
      return Status;
    }
    std::copy_n(Code.begin() + Pos, Size, std::back_inserter(Str));
    Pos += Size;
  }
  return Status;
}

} // namespace SSVM
