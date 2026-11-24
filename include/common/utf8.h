// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/utf8.h - UTF-8 validation -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the UTF-8 validation of a byte sequence. The binary
/// loader and the text format parser share it.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace WasmEdge {

/// Give the length of the UTF-8 sequence at Data[Pos], or 0 if the sequence
/// is not valid. An ASCII byte is a sequence of length 1. The function rejects
/// a lead byte that is not valid, a sequence that the data cuts short, a
/// continuation byte that is not valid, an overlong encoding, a code point
/// that is more than U+10FFFF, and a surrogate.
inline size_t utf8SequenceLength(std::string_view Data, size_t Pos) noexcept {
  const auto B = static_cast<uint8_t>(Data[Pos]);
  size_t Len = 0;
  uint32_t CodePoint = 0;
  if (B <= 0x7F) {
    return 1;
  }
  if ((B & 0xE0) == 0xC0) {
    Len = 2;
    CodePoint = B & 0x1F;
  } else if ((B & 0xF0) == 0xE0) {
    Len = 3;
    CodePoint = B & 0x0F;
  } else if ((B & 0xF8) == 0xF0) {
    Len = 4;
    CodePoint = B & 0x07;
  } else {
    return 0;
  }
  if (Pos + Len > Data.size()) {
    return 0;
  }
  for (size_t J = 1; J < Len; ++J) {
    const auto C = static_cast<uint8_t>(Data[Pos + J]);
    if ((C & 0xC0) != 0x80) {
      return 0;
    }
    CodePoint = (CodePoint << 6) | (C & 0x3F);
  }
  if ((Len == 2 && CodePoint < 0x80) || (Len == 3 && CodePoint < 0x800) ||
      (Len == 4 && CodePoint < 0x10000) || CodePoint > 0x10FFFF ||
      (CodePoint >= 0xD800 && CodePoint <= 0xDFFF)) {
    return 0;
  }
  return Len;
}

/// Return true if Data is valid UTF-8.
inline bool isValidUTF8(std::string_view Data) noexcept {
  size_t I = 0;
  while (I < Data.size()) {
    const size_t Len = utf8SequenceLength(Data, I);
    if (Len == 0) {
      return false;
    }
    I += Len;
  }
  return true;
}

} // namespace WasmEdge
