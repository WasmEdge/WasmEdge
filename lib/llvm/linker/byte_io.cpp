// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/byte_io.h"

#include <llvm/Support/LEB128.h>
#include <llvm/Support/MathExtras.h>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

namespace {

constexpr uint8_t BitsPerByte = 8;
constexpr uint8_t ByteField = 1;
constexpr uint8_t HalfField = 2;
constexpr uint8_t WordField = 4;
constexpr uint8_t DoubleWordField = 8;

constexpr bool validWidth(uint8_t Width) noexcept {
  return Width == ByteField || Width == HalfField || Width == WordField ||
         Width == DoubleWordField;
}

template <typename T> Expect<T> fieldError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

} // namespace

bool validField(Span<const Byte> Bytes, uint64_t Offset,
                uint8_t Width) noexcept {
  return validWidth(Width) && Offset <= Bytes.size() &&
         Width <= Bytes.size() - Offset;
}

Expect<uint64_t> readUnsigned(Span<const Byte> Bytes, uint64_t Offset,
                              uint8_t Width, Endianness Endian) noexcept {
  if (!validField(Bytes, Offset, Width)) {
    return fieldError<uint64_t>();
  }
  return getInteger(Bytes, Offset, Width, Endian);
}

Expect<int64_t> readSigned(Span<const Byte> Bytes, uint64_t Offset,
                           uint8_t Width, Endianness Endian) noexcept {
  auto Value = readUnsigned(Bytes, Offset, Width, Endian);
  if (!Value) {
    return fieldError<int64_t>();
  }
  return signExtend(*Value, Width * BitsPerByte);
}

Expect<void> writeUnsigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                           Endianness Endian, uint64_t Value) noexcept {
  if (!validField(Span<const Byte>(Bytes.data(), Bytes.size()), Offset,
                  Width) ||
      (Width < DoubleWordField &&
       Value >= (UINT64_C(1) << (Width * BitsPerByte)))) {
    return fieldError<void>();
  }
  putInteger(Bytes, Offset, Value, Width, Endian);
  return {};
}

Expect<void> writeSigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                         Endianness Endian, int64_t Value) noexcept {
  if (!validWidth(Width)) {
    return fieldError<void>();
  }
  uint64_t Bits = static_cast<uint64_t>(Value);
  if (Width < DoubleWordField) {
    const unsigned FieldBits = Width * BitsPerByte;
    if (!llvm::isIntN(FieldBits, Value)) {
      return fieldError<void>();
    }
    Bits &= (UINT64_C(1) << FieldBits) - 1;
  }
  return writeUnsigned(Bytes, Offset, Width, Endian, Bits);
}

void appendULEB128(std::vector<Byte> &Bytes, uint64_t Value) {
  uint8_t Encoded[16];
  const unsigned Size = llvm::encodeULEB128(Value, Encoded);
  Bytes.insert(Bytes.end(), Encoded, Encoded + Size);
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
