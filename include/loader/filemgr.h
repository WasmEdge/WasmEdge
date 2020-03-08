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

#include "common/errcode.h"
#include "common/value.h"

#include <fstream>
#include <string>
#include <vector>

namespace SSVM {

/// File manager interface.
class FileMgr {
public:
  /// Set the file path.
  virtual Expect<void> setPath(const std::string &FilePath) = 0;

  /// Set the binary data.
  virtual Expect<void> setCode(const Bytes &CodeData) = 0;

  /// Read one byte.
  virtual Expect<Byte> readByte() = 0;

  /// Read number of bytes into a vector.
  virtual Expect<Bytes> readBytes(size_t SizeToRead) = 0;

  /// Read an unsigned int.
  virtual Expect<uint32_t> readU32() = 0;

  /// Read an unsigned long long int.
  virtual Expect<uint64_t> readU64() = 0;

  /// Read a signed int.
  virtual Expect<int32_t> readS32() = 0;

  /// Read a signed long long int.
  virtual Expect<int64_t> readS64() = 0;

  /// Read a float.
  virtual Expect<float> readF32() = 0;

  /// Read a double.
  virtual Expect<double> readF64() = 0;

  /// Read a string, which is size(unsigned int) + bytes.
  virtual Expect<std::string> readName() = 0;

protected:
  /// File manager status.
  ErrCode Status = ErrCode::InvalidPath;
};

/// File stream version of file manager.
class FileMgrFStream : public FileMgr {
public:
  FileMgrFStream() = default;
  virtual ~FileMgrFStream();

  /// Inheritted from FileMgr.
  virtual Expect<void> setPath(const std::string &FilePath);
  virtual Expect<void> setCode(const Bytes &CodeData) {
    return Unexpect(ErrCode::InvalidPath);
  }
  virtual Expect<Byte> readByte();
  virtual Expect<Bytes> readBytes(size_t SizeToRead);
  virtual Expect<uint32_t> readU32();
  virtual Expect<uint64_t> readU64();
  virtual Expect<int32_t> readS32();
  virtual Expect<int64_t> readS64();
  virtual Expect<float> readF32();
  virtual Expect<double> readF64();
  virtual Expect<std::string> readName();

private:
  /// file stream.
  std::ifstream Fin;
};

/// Vector version of file manager.
class FileMgrVector : public FileMgr {
public:
  FileMgrVector() = default;
  virtual ~FileMgrVector() = default;

  /// Inheritted from FileMgr.
  virtual Expect<void> setPath(const std::string &FilePath) {
    return Unexpect(ErrCode::InvalidPath);
  }
  virtual Expect<void> setCode(const Bytes &CodeData);
  virtual Expect<Byte> readByte();
  virtual Expect<Bytes> readBytes(size_t SizeToRead);
  virtual Expect<uint32_t> readU32();
  virtual Expect<uint64_t> readU64();
  virtual Expect<int32_t> readS32();
  virtual Expect<int64_t> readS64();
  virtual Expect<float> readF32();
  virtual Expect<double> readF64();
  virtual Expect<std::string> readName();

  uint32_t getRemainSize() const { return Code.size() - Pos; }
  void clearBuffer() {
    Code.clear();
    Pos = 0;
    Status = ErrCode::EndOfFile;
  }

private:
  /// Reference to input vector.
  Bytes Code;
  uint32_t Pos = 0;
};

} // namespace SSVM
