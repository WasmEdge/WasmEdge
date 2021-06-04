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
  uint64_t getOffset() const noexcept { return Pos; }

  /// Get last succeeded read offset.
  uint64_t getLastOffset() const noexcept { return LastPos; }

  /// Get remain size.
  uint64_t getRemainSize() const noexcept { return Size - Pos; }

  /// Set limit read section size.
  void setSectionSize(uint64_t SecSize) {
    if (likely(UINT64_MAX - Pos >= SecSize)) {
      SecPos = std::min(Pos + SecSize, Size);
    } else {
      SecPos = std::min(UINT64_MAX - Pos, Size);
    }
  }

  /// Unset limit read section size.
  void unsetSectionSize() { SecPos.reset(); }

  /// Reset status
  void reset() {
    Status = ErrCode::UnexpectedEnd;
    LastPos = 0;
    Pos = 0;
    Size = 0;
    Data = nullptr;
    FileMap.reset();
    DataHolder.reset();
  }

private:
  /// Helper function for reading number of bytes into a vector.
  Expect<void> readBytes(Span<Byte> Buffer);

  /// Helper function for checking boundary.
  Expect<void> testRead(uint64_t Read);

  /// File manager status.
  ErrCode Status = ErrCode::UnexpectedEnd;

  /// Last succeeded read start or read failed offset.
  /// Will be set to the read error or EOF offset when read failed, or set to
  /// the u32, u64, s32, s64, f32, f64, name, or bytes start offset when read
  /// succeeded or syntax error.
  uint64_t LastPos;

  /// Section limit offset. If a value is set, it will return an 'UnexpectedEnd'
  /// if the read offset cross this value.
  std::optional<uint64_t> SecPos;

  /// Current read offset.
  uint64_t Pos;

  /// File or vector size.
  uint64_t Size;

  /// File or data management.
  const Byte *Data;
  std::optional<MMap> FileMap;
  std::optional<std::vector<Byte>> DataHolder;
};

} // namespace WasmEdge
