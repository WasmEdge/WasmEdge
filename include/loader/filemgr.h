#pragma once

#include "loader.h"
#include <fstream>
#include <string>
#include <vector>

class FileMgr {
public:
  virtual Loader::ErrCode setPath(const std::string &FilePath) = 0;
  virtual Loader::ErrCode readByte(unsigned char &Byte) = 0;
  virtual Loader::ErrCode readBytes(std::vector<unsigned char> &Buf,
                                    size_t SizeToRead) = 0;
  virtual Loader::ErrCode readU32(uint32_t &U32) = 0;
  virtual Loader::ErrCode readU64(uint64_t &U64) = 0;
  virtual Loader::ErrCode readS32(int32_t &S32) = 0;
  virtual Loader::ErrCode readS64(int64_t &S64) = 0;
  virtual Loader::ErrCode readF32(float &F32) = 0;
  virtual Loader::ErrCode readF64(double &F64) = 0;
  virtual Loader::ErrCode readName(std::string &Str) = 0;

protected:
  std::string Path;
  Loader::ErrCode Status = Loader::ErrCode::InvalidPath;
};

class FileMgrFStream : public FileMgr {
public:
  FileMgrFStream() = default;
  virtual ~FileMgrFStream();
  virtual Loader::ErrCode setPath(const std::string &FilePath);
  virtual Loader::ErrCode readByte(unsigned char &Byte);
  virtual Loader::ErrCode readBytes(std::vector<unsigned char> &Buf,
                                    size_t SizeToRead);
  virtual Loader::ErrCode readU32(uint32_t &U32);
  virtual Loader::ErrCode readU64(uint64_t &U64);
  virtual Loader::ErrCode readS32(int32_t &S32);
  virtual Loader::ErrCode readS64(int64_t &S64);
  virtual Loader::ErrCode readF32(float &F32);
  virtual Loader::ErrCode readF64(double &F64);
  virtual Loader::ErrCode readName(std::string &Str);

private:
  std::ifstream Fin;
};