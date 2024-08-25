// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "common/types.h"
#include "system/mmap.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {

/// File manager interface.
class FileMgr {
public:
  enum class FileHeader : uint8_t {
    // WASM or universal WASM.
    Wasm,
    // AOT compiled WASM as Linux ELF.
    ELF,
    // AOT compiled WASM as MacOS Mach_O 32-bit.
    MachO_32,
    // AOT compiled WASM as MacOS Mach_O 64-bit.
    MachO_64,
    // AOT compiled WASM as Windows DLL.
    DLL,
    // Unknown file header.
    Unknown
  };

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

  template <typename Ret, size_t N> Expect<Ret> readSN();

  /// Read a signed int.
  Expect<int32_t> readS32();

  /// Read a S33.
  Expect<int64_t> readS33();

  /// Read a signed long long int.
  Expect<int64_t> readS64();

  /// Read a float.
  Expect<float> readF32();

  /// Read a double.
  Expect<double> readF64();

  /// Read a string, which is size(unsigned int) + bytes.
  Expect<std::string> readName();

  /// Peek one byte.
  Expect<Byte> peekByte();

  /// Get the file header type.
  FileHeader getHeaderType();

  /// Get current offset.
  uint64_t getOffset() const noexcept { return Pos; }

  /// Get last succeeded read offset.
  uint64_t getLastOffset() const noexcept { return LastPos; }

  /// Get remain size.
  uint64_t getRemainSize() const noexcept { return Size - Pos; }

  /// Jump the content with size (size + content).
  Expect<void> jumpContent();

  /// Change the access position of the file.
  void seek(uint64_t NewPos) {
    if (Status != ErrCode::Value::IllegalPath) {
      Pos = std::min(NewPos, Size);
      LastPos = Pos;
      Status = ErrCode::Value::Success;
    }
  }

  /// Reset status
  void reset() {
    Status = ErrCode::Value::UnexpectedEnd;
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
  ErrCode::Value Status = ErrCode::Value::UnexpectedEnd;

  /// Last succeeded read start or read failed offset.
  /// Will be set to the read error or EOF offset when read failed, or set to
  /// the u32, u64, s32, s64, f32, f64, name, or bytes start offset when read
  /// succeeded or syntax error.
  uint64_t LastPos;

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
