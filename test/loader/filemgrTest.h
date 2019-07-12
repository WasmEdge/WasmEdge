#pragma once

#include "loader/filemgr.h"
#include <queue>

class FileMgrTest : public AST::FileMgr {
public:
  FileMgrTest() = default;
  virtual ~FileMgrTest(){};
  virtual bool setPath(const std::string &FilePath) { return false; }
  bool setVector(std::vector<unsigned char> &Vec) {
    for (auto it = Vec.begin(); it != Vec.end(); it++)
      Que.push(*it);
    Vec.clear();
    return true;
  }

  virtual bool readByte(unsigned char &Byte) {
    if (Que.size() == 0)
      return false;
    Byte = Que.front();
    Que.pop();
    return true;
  }

  virtual bool readBytes(std::vector<unsigned char> &Buf, size_t SizeToRead) {
    if (Que.size() < SizeToRead)
      return false;
    while (SizeToRead--) {
      Buf.push_back(Que.front());
      Que.pop();
    }
    return true;
  }

  virtual bool readU32(uint32_t &U32) {
    uint32_t Result = 0;
    uint32_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Result |= (Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    U32 = Result;
    return true;
  }

  virtual bool readU64(uint64_t &U64) {
    uint64_t Result = 0;
    uint64_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Result |= static_cast<uint64_t>(Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    U64 = Result;
    return true;
  };

  virtual bool readS32(int32_t &S32) {
    int32_t Result = 0;
    uint32_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Result |= (Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    if (Byte & 0x40U && Offset < 32) {
      Result |= 0xFFFFFFFF << Offset;
    }
    S32 = Result;
    return true;
  }

  virtual bool readS64(int64_t &S64) {
    int64_t Result = 0;
    uint64_t Offset = 0;
    unsigned char Byte = 0x80;
    while (Byte & 0x80U) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Result |= static_cast<int64_t>(Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    if (Byte & 0x40U && Offset < 64) {
      Result |= 0xFFFFFFFFFFFFFFFFL << Offset;
    }
    S64 = Result;
    return true;
  }

  virtual bool readF32(float &F32) {
    union {
      uint32_t U;
      float F;
    } Val;
    Val.U = 0;
    unsigned char Byte = 0x00;
    for (int i = 0; i < 4; i++) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Val.U |= (Byte & 0xFFU) << (i * 8);
    }
    F32 = Val.F;
    return true;
  }

  virtual bool readF64(double &F64) {
    union {
      uint64_t U;
      double D;
    } Val;
    Val.U = 0;
    unsigned char Byte = 0x00;
    for (int i = 0; i < 8; i++) {
      if (Que.size() == 0)
        return false;
      Byte = Que.front();
      Que.pop();
      Val.U |= static_cast<uint64_t>(Byte & 0xFFU) << (i * 8);
    }
    F64 = Val.D;
    return true;
  }

  virtual bool readName(std::string &Str) {
    unsigned int Size = 0;
    if (!readU32(Size))
      return false;
    if (Size > 0) {
      if (Que.size() < Size)
        return false;
      while (Size--) {
        Str.push_back(Que.front());
        Que.pop();
      }
    }
    return true;
  };

private:
  std::queue<unsigned char> Que;
};