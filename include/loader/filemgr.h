#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace AST {

class FileMgr {
public:
  enum class ErrCode : unsigned int {
    Success = 0,
    InvalidPath,
    ReadError,
    EndOfFile
  };
  virtual ErrCode setPath(const std::string &FilePath) = 0;
  virtual ErrCode readByte(unsigned char &Byte) = 0;
  virtual ErrCode readBytes(std::vector<unsigned char> &Buf,
                            size_t SizeToRead) = 0;
  virtual ErrCode readU32(uint32_t &U32) = 0;
  virtual ErrCode readU64(uint64_t &U64) = 0;
  virtual ErrCode readS32(int32_t &S32) = 0;
  virtual ErrCode readS64(int64_t &S64) = 0;
  virtual ErrCode readF32(float &F32) = 0;
  virtual ErrCode readF64(double &F64) = 0;
  virtual ErrCode readName(std::string &Str) = 0;

protected:
  std::string Path;
  ErrCode Status = ErrCode::InvalidPath;
};

class FileMgrFStream : public FileMgr {
public:
  FileMgrFStream() = default;
  virtual ~FileMgrFStream();
  virtual ErrCode setPath(const std::string &FilePath);
  virtual ErrCode readByte(unsigned char &Byte);
  virtual ErrCode readBytes(std::vector<unsigned char> &Buf, size_t SizeToRead);
  virtual ErrCode readU32(uint32_t &U32);
  virtual ErrCode readU64(uint64_t &U64);
  virtual ErrCode readS32(int32_t &S32);
  virtual ErrCode readS64(int64_t &S64);
  virtual ErrCode readF32(float &F32);
  virtual ErrCode readF64(double &F64);
  virtual ErrCode readName(std::string &Str);

protected:
  std::string Path;

private:
  std::ifstream Fin;
};

} // namespace AST