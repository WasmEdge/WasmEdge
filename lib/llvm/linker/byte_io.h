// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "linker/link_graph.h"

#include <llvm/Support/CheckedArithmetic.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

bool validField(Span<const Byte> Bytes, uint64_t Offset,
                uint8_t Width) noexcept;
Expect<uint64_t> readUnsigned(Span<const Byte> Bytes, uint64_t Offset,
                              uint8_t Width, Endianness Endian) noexcept;
Expect<int64_t> readSigned(Span<const Byte> Bytes, uint64_t Offset,
                           uint8_t Width, Endianness Endian) noexcept;
Expect<void> writeUnsigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                           Endianness Endian, uint64_t Value) noexcept;
Expect<void> writeSigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                         Endianness Endian, int64_t Value) noexcept;
void appendULEB128(std::vector<Byte> &Bytes, uint64_t Value);

inline bool addUnsigned(uint64_t Left, uint64_t Right,
                        uint64_t &Result) noexcept {
  const auto Value = llvm::checkedAddUnsigned(Left, Right);
  if (!Value)
    return false;
  Result = *Value;
  return true;
}

inline bool checkedMul(uint64_t Left, uint64_t Right,
                       uint64_t &Result) noexcept {
  const auto Value = llvm::checkedMulUnsigned(Left, Right);
  if (!Value)
    return false;
  Result = *Value;
  return true;
}

inline bool checkedAlign(uint64_t Value, uint64_t Alignment,
                         uint64_t &Result) noexcept {
  const uint64_t Mask = Alignment - 1;
  if (!addUnsigned(Value, Mask, Result))
    return false;
  Result &= ~Mask;
  return true;
}

inline bool fitsSizeT(uint64_t Value) noexcept {
#if SIZE_MAX < UINT64_MAX
  return Value <= SIZE_MAX;
#else
  static_cast<void>(Value);
  return true;
#endif
}

inline void putInteger(Span<Byte> Bytes, uint64_t Offset, uint64_t Value,
                       uint8_t Width,
                       Endianness Endian = Endianness::Little) noexcept {
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Bytes[Offset + I] = static_cast<Byte>(Value >> (Shift * 8));
  }
}

inline uint64_t getInteger(Span<const Byte> Bytes, uint64_t Offset,
                           uint8_t Width,
                           Endianness Endian = Endianness::Little) noexcept {
  uint64_t Result = 0;
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (Shift * 8);
  }
  return Result;
}

inline bool addSigned(uint64_t Base, int64_t Delta, uint64_t &Result) noexcept {
  if (Delta >= 0)
    return addUnsigned(Base, static_cast<uint64_t>(Delta), Result);
  const uint64_t Magnitude = static_cast<uint64_t>(-(Delta + 1)) + 1;
  if (Magnitude > Base)
    return false;
  Result = Base - Magnitude;
  return true;
}

inline bool addSigned(int64_t Left, int64_t Right, int64_t &Result) noexcept {
  const auto Value = llvm::checkedAdd(Left, Right);
  if (!Value)
    return false;
  Result = *Value;
  return true;
}

/// Reads the low Bits of Raw as a two's complement value. Bits is 1 to 64.
constexpr int64_t signExtend(uint64_t Raw, unsigned Bits) noexcept {
  const uint64_t Mask =
      Bits >= 64 ? ~UINT64_C(0) : (UINT64_C(1) << Bits) - UINT64_C(1);
  const uint64_t Value = Raw & Mask;
  if ((Value & (UINT64_C(1) << (Bits - 1))) == 0)
    return static_cast<int64_t>(Value);
  const uint64_t Magnitude = ((~Value) & Mask) + 1;
  return Magnitude > static_cast<uint64_t>(INT64_MAX)
             ? INT64_MIN
             : -static_cast<int64_t>(Magnitude);
}

/// Divides with rounding toward negative infinity. Divisor must be positive.
constexpr int64_t floorDivide(int64_t Value, int64_t Divisor) noexcept {
  const int64_t Quotient = Value / Divisor;
  return Value % Divisor < 0 ? Quotient - 1 : Quotient;
}

inline bool signedDelta(uint64_t Left, uint64_t Right,
                        int64_t &Result) noexcept {
  if (Left >= Right) {
    const uint64_t Difference = Left - Right;
    if (Difference > static_cast<uint64_t>(INT64_MAX))
      return false;
    Result = static_cast<int64_t>(Difference);
    return true;
  }
  const uint64_t Difference = Right - Left;
  const uint64_t MinimumMagnitude = UINT64_C(1) << 63;
  if (Difference > MinimumMagnitude)
    return false;
  Result = Difference == MinimumMagnitude ? INT64_MIN
                                          : -static_cast<int64_t>(Difference);
  return true;
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
