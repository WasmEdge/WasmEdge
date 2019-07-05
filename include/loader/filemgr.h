#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace AST {

class FileMgr {
public:
  virtual bool setPath(const std::string &FilePath) = 0;
  virtual bool readByte(unsigned char &Byte) = 0;
  virtual bool readU32(uint32_t &U32) = 0;
  virtual bool readU64(uint64_t &U64) = 0;
  virtual bool readS32(int32_t &S32) = 0;
  virtual bool readS64(int64_t &S64) = 0;
  virtual bool readF32(float &F32) = 0;
  virtual bool readF64(double &F64) = 0;
  virtual bool readSize(std::vector<unsigned char> &Buf, size_t SizeToRead) = 0;
  virtual bool readName(std::string &Str) = 0;

protected:
  std::string Path;
};

class FileMgrF : public FileMgr {
public:
  FileMgrF() = default;
  virtual ~FileMgrF();
  virtual bool setPath(const std::string &FilePath);
  virtual bool readByte(unsigned char &Byte);
  virtual bool readU32(uint32_t &U32);
  virtual bool readU64(uint64_t &U64);
  virtual bool readS32(int32_t &S32);
  virtual bool readS64(int64_t &S64);
  virtual bool readF32(float &F32);
  virtual bool readF64(double &F64);
  virtual bool readSize(std::vector<unsigned char> &Buf, size_t SizeToRead);
  virtual bool readName(std::string &Str);

protected:
  std::string Path;

private:
  std::ifstream Fin;
};

} // namespace AST