// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/loader/filemgr.h - File Manager definition -------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the FileMgr class, which controls flow
/// of WASM loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include <fstream>
#include <string>
#include <vector>

namespace SSVM {

/// File manager interface.
class FileMgr {
public:
  /// Set the file path.
  virtual Loader::ErrCode setPath(const std::string &FilePath) = 0;

  /// Read one byte.
  virtual Loader::ErrCode readByte(unsigned char &Byte) = 0;

  /// Read number of bytes into a vector.
  virtual Loader::ErrCode readBytes(std::vector<unsigned char> &Buf,
                                    size_t SizeToRead) = 0;

  /// Read an unsigned int.
  virtual Loader::ErrCode readU32(uint32_t &U32) = 0;

  /// Read an unsigned long long int.
  virtual Loader::ErrCode readU64(uint64_t &U64) = 0;

  /// Read a signed int.
  virtual Loader::ErrCode readS32(int32_t &S32) = 0;

  /// Read a signed long long int.
  virtual Loader::ErrCode readS64(int64_t &S64) = 0;

  /// Read a float.
  virtual Loader::ErrCode readF32(float &F32) = 0;

  /// Read a double.
  virtual Loader::ErrCode readF64(double &F64) = 0;

  /// Read a string, which is size(unsigned int) + bytes.
  virtual Loader::ErrCode readName(std::string &Str) = 0;

protected:
  /// The file path string.
  std::string Path;

  /// File manager status.
  Loader::ErrCode Status = Loader::ErrCode::InvalidPath;
};

/// File stream version of file manager.
class FileMgrFStream : public FileMgr {
public:
  FileMgrFStream() = default;
  virtual ~FileMgrFStream();

  /// Inheritted from FileMgr.
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
  /// file stream.
  std::ifstream Fin;
};

} // namespace SSVM
