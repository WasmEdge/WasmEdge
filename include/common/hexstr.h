// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/hexstr.h - Hex string formation -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about hex and string conversions.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {

uint8_t convertCharToHex(const char C);

void convertBytesToHexStr(Span<const uint8_t> Src, std::string &Dst,
                          const uint32_t Padding = 0,
                          const bool IsLittleEndian = false);

void convertValVecToHexStr(Span<const uint8_t> Src, std::string &Dst,
                           const uint32_t Padding = 0);

void convertHexStrToBytes(std::string_view Src, std::vector<uint8_t> &Dst,
                          uint32_t Padding = 2,
                          const bool IsLittleEndian = false);

void convertHexStrToValVec(std::string_view Src, std::vector<uint8_t> &Dst,
                           const uint32_t Padding = 2);

std::string convertUIntToHexStr(const uint64_t Num, uint32_t MinLen = 8);

} // namespace WasmEdge
