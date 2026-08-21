// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/loader/serialize_writer.h - Encoding primitives ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the primitives encoding scalars into the WASM binary
/// format. They carry no context and perform no validation.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Loader {

template <typename NumType, size_t N>
inline void writeUN(NumType Num, std::vector<uint8_t> &OutVec,
                    std::vector<uint8_t>::iterator It) noexcept {
  uint8_t Buf[N / 7 + 1];
  uint32_t Len = 0;
  do {
    uint8_t X = std::make_unsigned_t<NumType>(Num) & 0x7FU;
    Num >>= 7;
    if (Num) {
      X |= 0x80U;
    }
    Buf[Len] = X;
    Len++;
  } while (Num);
  OutVec.insert(It, Buf, Buf + Len);
}

template <typename NumType, size_t N>
inline void writeSN(NumType Num, std::vector<uint8_t> &OutVec) noexcept {
  uint8_t Buf[N / 7 + 1];
  uint32_t Len = 0;
  bool More = true;
  while (More) {
    uint8_t X = static_cast<std::make_unsigned_t<NumType>>(Num) & 0x7FU;
    Num >>= 7;
    if ((Num == 0 && !(X & 0x40)) || (Num == -1 && X & 0x40)) {
      More = false;
    } else {
      X |= 0x80;
    }
    Buf[Len] = X;
    Len++;
  }
  OutVec.insert(OutVec.end(), Buf, Buf + Len);
}

template <typename NumType, typename IntType>
inline typename std::enable_if_t<
    sizeof(NumType) == sizeof(IntType) && std::is_integral_v<IntType>, void>
writeFN(NumType Num, std::vector<uint8_t> &OutVec) noexcept {
  std::make_unsigned_t<IntType> Buf = 0;
  std::memcpy(&Buf, &Num, sizeof(NumType));
  // Force conversion into little endian.
  for (uint32_t I = 0; I < sizeof(NumType); I++) {
    OutVec.push_back(static_cast<uint8_t>(Buf & 0xFFU));
    Buf = Buf >> 8;
  }
}

inline void writeU32(uint32_t Num, std::vector<uint8_t> &OutVec,
                     std::vector<uint8_t>::iterator It) noexcept {
  writeUN<uint32_t, 32>(Num, OutVec, It);
}
inline void writeU32(uint32_t Num, std::vector<uint8_t> &OutVec) noexcept {
  writeUN<uint32_t, 32>(Num, OutVec, OutVec.end());
}
inline void writeU64(uint64_t Num, std::vector<uint8_t> &OutVec) noexcept {
  writeUN<uint64_t, 64>(Num, OutVec, OutVec.end());
}
inline void writeS32(int32_t Num, std::vector<uint8_t> &OutVec) noexcept {
  writeSN<int32_t, 32>(Num, OutVec);
}
inline void writeS33(int64_t Num, std::vector<uint8_t> &OutVec) noexcept {
  writeSN<int64_t, 33>(Num, OutVec);
}
inline void writeS64(int64_t Num, std::vector<uint8_t> &OutVec) noexcept {
  writeSN<int64_t, 64>(Num, OutVec);
}
inline void writeF32(float Num, std::vector<uint8_t> &OutVec) noexcept {
  writeFN<float, uint32_t>(Num, OutVec);
}
inline void writeF64(double Num, std::vector<uint8_t> &OutVec) noexcept {
  writeFN<double, uint64_t>(Num, OutVec);
}

inline void writeName(std::string_view Name,
                      std::vector<uint8_t> &OutVec) noexcept {
  writeU32(static_cast<uint32_t>(Name.size()), OutVec);
  OutVec.insert(OutVec.end(), Name.begin(), Name.end());
}

template <typename T>
inline void writeBytes(const T &Bytes, std::vector<uint8_t> &OutVec) noexcept {
  OutVec.insert(OutVec.end(), Bytes.begin(), Bytes.end());
}

} // namespace Loader
} // namespace WasmEdge
