#pragma once

#include "loader/filemgr.h"
#include <queue>

class FileMgrTest : public AST::FileMgr {
public:
  FileMgrTest() = default;
  virtual ~FileMgrTest(){};
  virtual ErrCode setPath(const std::string &FilePath) { return Status; }

  bool setVector(std::vector<unsigned char> &Vec) {
    for (auto it = Vec.begin(); it != Vec.end(); it++)
      Que.push(*it);
    if (Que.size() > 0)
      Status = ErrCode::Success;
    return true;
  }

  bool clearBuffer() {
    while (Que.size() > 0)
      Que.pop();
    return true;
  }

  size_t getQueueSize() { return Que.size(); }

  virtual ErrCode readByte(unsigned char &Byte) {
    if (Que.size() == 0) {
      Status = ErrCode::EndOfFile;
      return Status;
    } else {
      Status = ErrCode::Success;
    }
    Byte = Que.front();
    Que.pop();
    return Status;
  }

  virtual ErrCode readBytes(std::vector<unsigned char> &Buf,
                            size_t SizeToRead) {
    if (Que.size() < SizeToRead) {
      Status = ErrCode::EndOfFile;
      return Status;
    } else {
      Status = ErrCode::Success;
    }
    while (SizeToRead--) {
      Buf.push_back(Que.front());
      Que.pop();
    }
    return Status;
  }

  virtual ErrCode readU32(uint32_t &U32) {
    uint32_t Result = 0;
    uint32_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Result |= (Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    U32 = Result;
    return Status;
  }

  virtual ErrCode readU64(uint64_t &U64) {
    uint64_t Result = 0;
    uint64_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Result |= static_cast<uint64_t>(Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    U64 = Result;
    return Status;
  };

  virtual ErrCode readS32(int32_t &S32) {
    int32_t Result = 0;
    uint32_t Offset = 0;
    unsigned char Byte = 0x80U;
    while (Byte & 0x80U) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Result |= (Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    if (Byte & 0x40U && Offset < 32) {
      Result |= 0xFFFFFFFF << Offset;
    }
    S32 = Result;
    return Status;
  }

  virtual ErrCode readS64(int64_t &S64) {
    int64_t Result = 0;
    uint64_t Offset = 0;
    unsigned char Byte = 0x80;
    while (Byte & 0x80U) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Result |= static_cast<int64_t>(Byte & 0x7FU) << (Offset);
      Offset += 7;
    }
    if (Byte & 0x40U && Offset < 64) {
      Result |= 0xFFFFFFFFFFFFFFFFL << Offset;
    }
    S64 = Result;
    return Status;
  }

  virtual ErrCode readF32(float &F32) {
    union {
      uint32_t U;
      float F;
    } Val;
    Val.U = 0;
    unsigned char Byte = 0x00;
    for (int i = 0; i < 4; i++) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Val.U |= (Byte & 0xFFU) << (i * 8);
    }
    F32 = Val.F;
    return Status;
  }

  virtual ErrCode readF64(double &F64) {
    union {
      uint64_t U;
      double D;
    } Val;
    Val.U = 0;
    unsigned char Byte = 0x00;
    for (int i = 0; i < 8; i++) {
      if (Que.size() == 0) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      Byte = Que.front();
      Que.pop();
      Val.U |= static_cast<uint64_t>(Byte & 0xFFU) << (i * 8);
    }
    F64 = Val.D;
    return Status;
  }

  virtual ErrCode readName(std::string &Str) {
    unsigned int Size = 0;
    if (readU32(Size) != ErrCode::Success)
      return Status;
    if (Size > 0) {
      if (Que.size() < Size) {
        Status = ErrCode::EndOfFile;
        return Status;
      } else {
        Status = ErrCode::Success;
      }
      while (Size--) {
        Str.push_back(Que.front());
        Que.pop();
      }
    }
    return Status;
  };

private:
  std::queue<unsigned char> Que;
};