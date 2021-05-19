// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/loader/filemgr.h - File Manager definition ---------------===//
//
// Part of the WasmEdge Project.
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
#include "common/filesystem.h"
#include "common/types.h"
#include "common/value.h"
#include "system/mmap.h"
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {

/// File manager interface.
class FileMgr {
public:
  /// Set the file path.
  Expect<void> setPath(const std::filesystem::path &FilePath);

  /// Set the binary data.
  Expect<void> setCode(Span<const Byte> CodeData);

  /// Set the binary data.
  Expect<void> setCode(std::vector<Byte> CodeData);

  /// Read one byte.
  Expect<Byte> readByte();

  /// Read number of bytes into a vector.
  Expect<std::vector<Byte>> readBytes(size_t SizeToRead);

  /// Read number of bytes into a vector.
  Expect<void> readBytes(Span<Byte> Buffer);

  /// Read an unsigned int.
  Expect<uint32_t> readU32();

  /// Read an unsigned long long int.
  Expect<uint64_t> readU64();

  /// Read a signed int.
  Expect<int32_t> readS32();

  /// Read a signed long long int.
  Expect<int64_t> readS64();

  /// Read a float.
  Expect<float> readF32();

  /// Read a double.
  Expect<double> readF64();

  /// Read a string, which is size(unsigned int) + bytes.
  Expect<std::string> readName();

  /// Get current offset.
  uint64_t getOffset() const { return Pos; }

  /// Get remain size.
  uint64_t getRemainSize() const { return Size - Pos; }

  /// Reset status
  void reset() {
    Status = ErrCode::EndOfFile;
    Pos = 0;
    Size = 0;
    Data = nullptr;
    Map.reset();
    DataHolder.reset();
  }

private:
  /// File manager status.
  ErrCode Status = ErrCode::EndOfFile;
  uint64_t Pos;
  uint64_t Size;
  const Byte *Data;
  std::optional<MMap> Map;
  std::optional<std::vector<Byte>> DataHolder;
};

} // namespace WasmEdge
