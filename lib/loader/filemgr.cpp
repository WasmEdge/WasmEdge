#include "loader/filemgr.h"
#include <iterator>

namespace AST {

FileMgrF::~FileMgrF() {
  if (Fin.is_open()) {
    Fin.close();
  }
}

bool FileMgrF::setPath(const std::string &FilePath) {
  if (Fin.is_open()) {
    Fin.close();
  }
  Path = FilePath;
  Fin.open(Path, std::ios::in | std::ios::binary);
  return !Fin.fail();
}

bool FileMgrF::readByte(unsigned char &Byte) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  char Buf = 0;
  Fin.get(Buf);
  if (Fin.fail())
    return false;
  Byte = static_cast<unsigned char>(Buf);
  return true;
}

bool FileMgrF::readU32(uint32_t &U32) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  uint32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    return false;
  U32 = Result;
  return true;
}

bool FileMgrF::readU64(uint64_t &U64) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  uint64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= static_cast<uint64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    return false;
  U64 = Result;
  return true;
}

bool FileMgrF::readS32(int32_t &S32) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  int32_t Result = 0;
  uint32_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= (Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    return false;
  if (Byte & 0x40) {
    Result |= 0xFFFFFFFF << Offset;
  }
  S32 = Result;
  return true;
}

bool FileMgrF::readS64(int64_t &S64) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  int64_t Result = 0;
  uint64_t Offset = 0;
  char Byte = 0x80;
  while (!Fin.fail() && Byte & 0x80) {
    Fin.get(Byte);
    Result |= static_cast<int64_t>(Byte & 0x7F) << (Offset);
    Offset += 7;
  }
  if (Fin.fail())
    return false;
  if (Byte & 0x40) {
    Result |= 0xFFFFFFFFFFFFFFFFL << Offset;
  }
  S64 = Result;
  return true;
}

bool FileMgrF::readF32(float &F32) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  union {
    uint32_t U;
    float F;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 4; i++) {
    Fin.get(Byte);
    if (Fin.fail())
      return false;
    Val.U |= (Byte & 0xFF) << (i * 8);
  }
  F32 = Val.F;
  return true;
}

bool FileMgrF::readF64(double &F64) {
  if (!Fin.is_open() || Fin.fail())
    return false;
  union {
    uint64_t U;
    double D;
  } Val;
  Val.U = 0;
  char Byte = 0x00;
  for (int i = 0; i < 8; i++) {
    Fin.get(Byte);
    if (Fin.fail())
      return false;
    Val.U |= static_cast<uint64_t>(Byte & 0xFF) << (i * 8);
  }
  F64 = Val.D;
  return true;
}

bool FileMgrF::readSize(std::vector<unsigned char> &Buf, size_t SizeToRead) {
  std::istreambuf_iterator<char> Iter(Fin);
  // TODO: error handling
  std::copy_n(Iter, SizeToRead, std::back_inserter(Buf));
  Iter++;
  return true;
}

bool FileMgrF::readName(std::string &Str) {
  unsigned int Size = 0;
  if (!readU32(Size))
    return false;
  std::istreambuf_iterator<char> Iter(Fin);
  std::copy_n(Iter, Size, std::back_inserter(Str));
  Iter++;
  return true;
}

} // namespace AST