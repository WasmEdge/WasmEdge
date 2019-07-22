#include "loader/filemgr.h"
#include <iterator>

namespace AST {

FileMgrFStream::~FileMgrFStream() {
  if (Fin.is_open()) {
    Fin.close();
  }
}

FileMgr::ErrCode FileMgrFStream::setPath(const std::string &FilePath) {
  if (Fin.is_open()) {
    Fin.close();
    Status = ErrCode::InvalidPath;
  }
  Path = FilePath;
  Fin.open(Path, std::ios::in | std::ios::binary);
  if (!Fin.fail())
    Status = ErrCode::Success;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readByte(unsigned char &Byte) {
  if (Status != ErrCode::Success)
    return Status;
  char Buf = 0;
  Fin.get(Buf);
  if (Fin.fail()) {
    Status = ErrCode::ReadError;
    return Status;
  }
  Byte = static_cast<unsigned char>(Buf);
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readBytes(std::vector<unsigned char> &Buf,
                                           size_t SizeToRead) {
  if (Status != ErrCode::Success)
    return Status;
  if (SizeToRead > 0) {
    std::istreambuf_iterator<char> Iter(Fin);
    // TODO: error handling
    std::copy_n(Iter, SizeToRead, std::back_inserter(Buf));
    Iter++;
  }
  if (Fin.fail())
    Status = ErrCode::ReadError;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readU32(uint32_t &U32) {
  if (Status != ErrCode::Success)
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
    Status = ErrCode::ReadError;
  else
    U32 = Result;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readU64(uint64_t &U64) {
  if (Status != ErrCode::Success)
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
    Status = ErrCode::ReadError;
  else
    U64 = Result;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readS32(int32_t &S32) {
  if (Status != ErrCode::Success)
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
    Status = ErrCode::ReadError;
  else {
    if (Byte & 0x40 && Offset < 32) {
      Result |= 0xFFFFFFFF << Offset;
    }
    S32 = Result;
  }
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readS64(int64_t &S64) {
  if (Status != ErrCode::Success)
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
    Status = ErrCode::ReadError;
  else {
    if (Byte & 0x40 && Offset < 64) {
      Result |= 0xFFFFFFFFFFFFFFFFL << Offset;
    }
    S64 = Result;
  }
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readF32(float &F32) {
  if (Status != ErrCode::Success)
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
      Status = ErrCode::ReadError;
      return Status;
    }
    Val.U |= (Byte & 0xFF) << (i * 8);
  }
  F32 = Val.F;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readF64(double &F64) {
  if (Status != ErrCode::Success)
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
      Status = ErrCode::ReadError;
      return Status;
    }
    Val.U |= static_cast<uint64_t>(Byte & 0xFF) << (i * 8);
  }
  F64 = Val.D;
  return Status;
}

FileMgr::ErrCode FileMgrFStream::readName(std::string &Str) {
  unsigned int Size = 0;
  if (readU32(Size) != ErrCode::Success)
    return Status;
  if (Size > 0) {
    std::istreambuf_iterator<char> Iter(Fin);
    std::copy_n(Iter, Size, std::back_inserter(Str));
    Iter++;
  }
  if (Fin.fail())
    Status = ErrCode::ReadError;
  return Status;
}

} // namespace AST