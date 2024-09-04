// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "common/filesystem.h"
#include "experimental/span.hpp"
#include "wasmedge/wasmedge.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

using namespace std::literals;

namespace {

std::vector<uint8_t> TestWasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x1d, 0x5,  0x60,
    0x0,  0x1,  0x7f, 0x60, 0x2,  0x6f, 0x7f, 0x1,  0x7f, 0x60, 0x2,  0x7f,
    0x7f, 0x1,  0x7f, 0x60, 0x2,  0x7f, 0x7f, 0x2,  0x7f, 0x7f, 0x60, 0x1,
    0x7f, 0x1,  0x7f, 0x2,  0x6f, 0x6,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x61, 0x64, 0x64, 0x0,  0x1,
    0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x73, 0x75, 0x62, 0x0,  0x1,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x6d, 0x75, 0x6c, 0x0,  0x1,
    0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x64, 0x69, 0x76, 0x0,  0x1,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x9,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x74, 0x65, 0x72, 0x6d, 0x0,
    0x0,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x9,  0x66, 0x75, 0x6e,
    0x63, 0x2d, 0x66, 0x61, 0x69, 0x6c, 0x0,  0x0,  0x3,  0xc,  0xb,  0x0,
    0x0,  0x0,  0x0,  0x2,  0x3,  0x4,  0x4,  0x4,  0x4,  0x4,  0x4,  0x7,
    0x2,  0x70, 0x0,  0xa,  0x6f, 0x0,  0xa,  0x5,  0x4,  0x1,  0x1,  0x1,
    0x3,  0x6,  0xf,  0x2,  0x7f, 0x1,  0x41, 0x8e, 0x1,  0xb,  0x7d, 0x0,
    0x43, 0xae, 0x47, 0x45, 0x44, 0xb,  0x7,  0xcd, 0x1,  0x10, 0x6,  0x66,
    0x75, 0x6e, 0x63, 0x2d, 0x31, 0x0,  0x6,  0x6,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x32, 0x0,  0x7,  0x6,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x33, 0x0,
    0x8,  0x6,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x34, 0x0,  0x9,  0x8,  0x66,
    0x75, 0x6e, 0x63, 0x2d, 0x61, 0x64, 0x64, 0x0,  0xa,  0xa,  0x66, 0x75,
    0x6e, 0x63, 0x2d, 0x6d, 0x75, 0x6c, 0x2d, 0x32, 0x0,  0xb,  0x12, 0x66,
    0x75, 0x6e, 0x63, 0x2d, 0x63, 0x61, 0x6c, 0x6c, 0x2d, 0x69, 0x6e, 0x64,
    0x69, 0x72, 0x65, 0x63, 0x74, 0x0,  0xc,  0xd,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x68, 0x6f, 0x73, 0x74, 0x2d, 0x61, 0x64, 0x64, 0x0,  0xd,  0xd,
    0x66, 0x75, 0x6e, 0x63, 0x2d, 0x68, 0x6f, 0x73, 0x74, 0x2d, 0x73, 0x75,
    0x62, 0x0,  0xe,  0xd,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x68, 0x6f, 0x73,
    0x74, 0x2d, 0x6d, 0x75, 0x6c, 0x0,  0xf,  0xd,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x68, 0x6f, 0x73, 0x74, 0x2d, 0x64, 0x69, 0x76, 0x0,  0x10, 0x8,
    0x74, 0x61, 0x62, 0x2d, 0x66, 0x75, 0x6e, 0x63, 0x1,  0x0,  0x7,  0x74,
    0x61, 0x62, 0x2d, 0x65, 0x78, 0x74, 0x1,  0x1,  0x3,  0x6d, 0x65, 0x6d,
    0x2,  0x0,  0xc,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x6d, 0x75, 0x74, 0x2d,
    0x69, 0x33, 0x32, 0x3,  0x0,  0xe,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x63,
    0x6f, 0x6e, 0x73, 0x74, 0x2d, 0x66, 0x33, 0x32, 0x3,  0x1,  0x9,  0xa,
    0x1,  0x0,  0x41, 0x2,  0xb,  0x4,  0x6,  0x7,  0x8,  0x9,  0xa,  0x5e,
    0xb,  0x4,  0x0,  0x41, 0x1,  0xb,  0x4,  0x0,  0x41, 0x2,  0xb,  0x4,
    0x0,  0x41, 0x3,  0xb,  0x4,  0x0,  0x41, 0x4,  0xb,  0x7,  0x0,  0x20,
    0x0,  0x20, 0x1,  0x6a, 0xb,  0xc,  0x0,  0x20, 0x0,  0x41, 0x2,  0x6c,
    0x20, 0x1,  0x41, 0x2,  0x6c, 0xb,  0x7,  0x0,  0x20, 0x0,  0x11, 0x0,
    0x0,  0xb,  0xa,  0x0,  0x41, 0x0,  0x25, 0x1,  0x20, 0x0,  0x10, 0x0,
    0xb,  0xa,  0x0,  0x41, 0x1,  0x25, 0x1,  0x20, 0x0,  0x10, 0x1,  0xb,
    0xa,  0x0,  0x41, 0x2,  0x25, 0x1,  0x20, 0x0,  0x10, 0x2,  0xb,  0xa,
    0x0,  0x41, 0x3,  0x25, 0x1,  0x20, 0x0,  0x10, 0x3,  0xb,  0xb,  0x10,
    0x1,  0x0,  0x41, 0xa,  0xb,  0xa,  0x0,  0x1,  0x2,  0x3,  0x4,  0x5,
    0x6,  0x7,  0x8,  0x9,  0x0,  0x8f, 0x2,  0x4,  0x6e, 0x61, 0x6d, 0x65,
    0x1,  0x8d, 0x1,  0x11, 0x0,  0x7,  0x65, 0x2d, 0x66, 0x2d, 0x61, 0x64,
    0x64, 0x1,  0x7,  0x65, 0x2d, 0x66, 0x2d, 0x73, 0x75, 0x62, 0x2,  0x7,
    0x65, 0x2d, 0x66, 0x2d, 0x6d, 0x75, 0x6c, 0x3,  0x7,  0x65, 0x2d, 0x66,
    0x2d, 0x64, 0x69, 0x76, 0x4,  0x8,  0x65, 0x2d, 0x66, 0x2d, 0x74, 0x65,
    0x72, 0x6d, 0x5,  0x8,  0x65, 0x2d, 0x66, 0x2d, 0x66, 0x61, 0x69, 0x6c,
    0x6,  0x3,  0x66, 0x2d, 0x31, 0x7,  0x3,  0x66, 0x2d, 0x32, 0x8,  0x3,
    0x66, 0x2d, 0x33, 0x9,  0x3,  0x66, 0x2d, 0x34, 0xa,  0x5,  0x66, 0x2d,
    0x61, 0x64, 0x64, 0xb,  0x7,  0x66, 0x2d, 0x6d, 0x75, 0x6c, 0x2d, 0x32,
    0xc,  0xa,  0x66, 0x2d, 0x63, 0x61, 0x6c, 0x6c, 0x2d, 0x69, 0x6e, 0x64,
    0xd,  0x7,  0x66, 0x2d, 0x65, 0x2d, 0x61, 0x64, 0x64, 0xe,  0x7,  0x66,
    0x2d, 0x65, 0x2d, 0x73, 0x75, 0x62, 0xf,  0x7,  0x66, 0x2d, 0x65, 0x2d,
    0x6d, 0x75, 0x6c, 0x10, 0x7,  0x66, 0x2d, 0x65, 0x2d, 0x64, 0x69, 0x76,
    0x2,  0x45, 0x11, 0x0,  0x2,  0x0,  0x0,  0x1,  0x0,  0x1,  0x2,  0x0,
    0x0,  0x1,  0x0,  0x2,  0x2,  0x0,  0x0,  0x1,  0x0,  0x3,  0x2,  0x0,
    0x0,  0x1,  0x0,  0x4,  0x0,  0x5,  0x0,  0x6,  0x0,  0x7,  0x0,  0x8,
    0x0,  0x9,  0x0,  0xa,  0x2,  0x0,  0x0,  0x1,  0x0,  0xb,  0x2,  0x0,
    0x0,  0x1,  0x0,  0xc,  0x1,  0x0,  0x0,  0xd,  0x1,  0x0,  0x0,  0xe,
    0x1,  0x0,  0x0,  0xf,  0x1,  0x0,  0x0,  0x10, 0x1,  0x0,  0x0,  0x4,
    0xf,  0x2,  0x0,  0x5,  0x74, 0x79, 0x70, 0x65, 0x30, 0x1,  0x5,  0x74,
    0x79, 0x70, 0x65, 0x31, 0x5,  0xb,  0x2,  0x0,  0x3,  0x74, 0x2d, 0x66,
    0x1,  0x3,  0x74, 0x2d, 0x65, 0x6,  0x4,  0x1,  0x0,  0x1,  0x6d, 0x7,
    0xd,  0x2,  0x0,  0x4,  0x67, 0x2d, 0x6d, 0x69, 0x1,  0x4,  0x67, 0x2d,
    0x63, 0x66};
std::vector<uint8_t> ImportWasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x34, 0xa,  0x60,
    0x0,  0x1,  0x7f, 0x60, 0x2,  0x6f, 0x7f, 0x1,  0x7f, 0x60, 0x1,  0x7c,
    0x0,  0x60, 0x2,  0x7c, 0x7e, 0x0,  0x60, 0x0,  0x0,  0x60, 0x4,  0x7f,
    0x7e, 0x7d, 0x7c, 0x0,  0x60, 0x1,  0x7d, 0x0,  0x60, 0x2,  0x7f, 0x7f,
    0x1,  0x7f, 0x60, 0x2,  0x7f, 0x7f, 0x2,  0x7f, 0x7f, 0x60, 0x1,  0x7f,
    0x1,  0x7f, 0x2,  0xa5, 0x2,  0x11, 0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x61, 0x64, 0x64, 0x0,  0x1,
    0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x73, 0x75, 0x62, 0x0,  0x1,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x6d, 0x75, 0x6c, 0x0,  0x1,
    0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x8,  0x66, 0x75, 0x6e, 0x63,
    0x2d, 0x64, 0x69, 0x76, 0x0,  0x1,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x9,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x74, 0x65, 0x72, 0x6d, 0x0,
    0x0,  0x6,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x9,  0x66, 0x75, 0x6e,
    0x63, 0x2d, 0x66, 0x61, 0x69, 0x6c, 0x0,  0x0,  0x5,  0x64, 0x75, 0x6d,
    0x6d, 0x79, 0x8,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x69, 0x33, 0x32, 0x3,
    0x7f, 0x1,  0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79, 0x8,  0x67, 0x6c, 0x6f,
    0x62, 0x2d, 0x69, 0x36, 0x34, 0x3,  0x7e, 0x0,  0x5,  0x64, 0x75, 0x6d,
    0x6d, 0x79, 0x8,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x66, 0x33, 0x32, 0x3,
    0x7d, 0x1,  0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79, 0x8,  0x67, 0x6c, 0x6f,
    0x62, 0x2d, 0x66, 0x36, 0x34, 0x3,  0x7c, 0x0,  0x5,  0x64, 0x75, 0x6d,
    0x6d, 0x79, 0x8,  0x74, 0x61, 0x62, 0x2d, 0x66, 0x75, 0x6e, 0x63, 0x1,
    0x70, 0x1,  0xa,  0x14, 0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79, 0x7,  0x74,
    0x61, 0x62, 0x2d, 0x65, 0x78, 0x74, 0x1,  0x6f, 0x1,  0xa,  0x1e, 0x5,
    0x64, 0x75, 0x6d, 0x6d, 0x79, 0x4,  0x6d, 0x65, 0x6d, 0x31, 0x2,  0x1,
    0x1,  0x3,  0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79, 0x4,  0x6d, 0x65, 0x6d,
    0x32, 0x2,  0x0,  0x2,  0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79, 0x4,  0x74,
    0x61, 0x67, 0x31, 0x4,  0x0,  0x2,  0x5,  0x64, 0x75, 0x6d, 0x6d, 0x79,
    0x4,  0x74, 0x61, 0x67, 0x32, 0x4,  0x0,  0x3,  0x5,  0x64, 0x75, 0x6d,
    0x6d, 0x79, 0x4,  0x74, 0x61, 0x67, 0x33, 0x4,  0x0,  0x4,  0x3,  0xc,
    0xb,  0x0,  0x0,  0x0,  0x0,  0x7,  0x8,  0x9,  0x9,  0x9,  0x9,  0x9,
    0x4,  0x7,  0x2,  0x70, 0x0,  0xa,  0x6f, 0x0,  0xa,  0x5,  0x4,  0x1,
    0x1,  0x1,  0x3,  0xd,  0x7,  0x3,  0x0,  0x5,  0x0,  0x4,  0x0,  0x6,
    0x6,  0xf,  0x2,  0x7f, 0x1,  0x41, 0x8e, 0x1,  0xb,  0x7d, 0x0,  0x43,
    0xae, 0x47, 0x45, 0x44, 0xb,  0x7,  0xe5, 0x1,  0x13, 0x6,  0x66, 0x75,
    0x6e, 0x63, 0x2d, 0x31, 0x0,  0x6,  0x6,  0x66, 0x75, 0x6e, 0x63, 0x2d,
    0x32, 0x0,  0x7,  0x6,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x33, 0x0,  0x8,
    0x6,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x34, 0x0,  0x9,  0x8,  0x66, 0x75,
    0x6e, 0x63, 0x2d, 0x61, 0x64, 0x64, 0x0,  0xa,  0xa,  0x66, 0x75, 0x6e,
    0x63, 0x2d, 0x6d, 0x75, 0x6c, 0x2d, 0x32, 0x0,  0xb,  0x12, 0x66, 0x75,
    0x6e, 0x63, 0x2d, 0x63, 0x61, 0x6c, 0x6c, 0x2d, 0x69, 0x6e, 0x64, 0x69,
    0x72, 0x65, 0x63, 0x74, 0x0,  0xc,  0xd,  0x66, 0x75, 0x6e, 0x63, 0x2d,
    0x68, 0x6f, 0x73, 0x74, 0x2d, 0x61, 0x64, 0x64, 0x0,  0xd,  0xd,  0x66,
    0x75, 0x6e, 0x63, 0x2d, 0x68, 0x6f, 0x73, 0x74, 0x2d, 0x73, 0x75, 0x62,
    0x0,  0xe,  0xd,  0x66, 0x75, 0x6e, 0x63, 0x2d, 0x68, 0x6f, 0x73, 0x74,
    0x2d, 0x6d, 0x75, 0x6c, 0x0,  0xf,  0xd,  0x66, 0x75, 0x6e, 0x63, 0x2d,
    0x68, 0x6f, 0x73, 0x74, 0x2d, 0x64, 0x69, 0x76, 0x0,  0x10, 0x8,  0x74,
    0x61, 0x62, 0x2d, 0x66, 0x75, 0x6e, 0x63, 0x1,  0x2,  0x7,  0x74, 0x61,
    0x62, 0x2d, 0x65, 0x78, 0x74, 0x1,  0x3,  0x3,  0x6d, 0x65, 0x6d, 0x2,
    0x2,  0x5,  0x74, 0x61, 0x67, 0x2d, 0x31, 0x4,  0x3,  0x5,  0x74, 0x61,
    0x67, 0x2d, 0x32, 0x4,  0x4,  0x5,  0x74, 0x61, 0x67, 0x2d, 0x33, 0x4,
    0x5,  0xc,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x6d, 0x75, 0x74, 0x2d, 0x69,
    0x33, 0x32, 0x3,  0x4,  0xe,  0x67, 0x6c, 0x6f, 0x62, 0x2d, 0x63, 0x6f,
    0x6e, 0x73, 0x74, 0x2d, 0x66, 0x33, 0x32, 0x3,  0x5,  0x9,  0xc,  0x1,
    0x2,  0x2,  0x41, 0x2,  0xb,  0x0,  0x4,  0x6,  0x7,  0x8,  0x9,  0xa,
    0x5e, 0xb,  0x4,  0x0,  0x41, 0x1,  0xb,  0x4,  0x0,  0x41, 0x2,  0xb,
    0x4,  0x0,  0x41, 0x3,  0xb,  0x4,  0x0,  0x41, 0x4,  0xb,  0x7,  0x0,
    0x20, 0x0,  0x20, 0x1,  0x6a, 0xb,  0xc,  0x0,  0x20, 0x0,  0x41, 0x2,
    0x6c, 0x20, 0x1,  0x41, 0x2,  0x6c, 0xb,  0x7,  0x0,  0x20, 0x0,  0x11,
    0x0,  0x2,  0xb,  0xa,  0x0,  0x41, 0x0,  0x25, 0x3,  0x20, 0x0,  0x10,
    0x0,  0xb,  0xa,  0x0,  0x41, 0x1,  0x25, 0x3,  0x20, 0x0,  0x10, 0x1,
    0xb,  0xa,  0x0,  0x41, 0x2,  0x25, 0x3,  0x20, 0x0,  0x10, 0x2,  0xb,
    0xa,  0x0,  0x41, 0x3,  0x25, 0x3,  0x20, 0x0,  0x10, 0x3,  0xb,  0xb,
    0x10, 0x1,  0x0,  0x41, 0xa,  0xb,  0xa,  0x0,  0x1,  0x2,  0x3,  0x4,
    0x5,  0x6,  0x7,  0x8,  0x9,  0x0,  0xe0, 0x1,  0x4,  0x6e, 0x61, 0x6d,
    0x65, 0x1,  0x8d, 0x1,  0x11, 0x0,  0x7,  0x65, 0x2d, 0x66, 0x2d, 0x61,
    0x64, 0x64, 0x1,  0x7,  0x65, 0x2d, 0x66, 0x2d, 0x73, 0x75, 0x62, 0x2,
    0x7,  0x65, 0x2d, 0x66, 0x2d, 0x6d, 0x75, 0x6c, 0x3,  0x7,  0x65, 0x2d,
    0x66, 0x2d, 0x64, 0x69, 0x76, 0x4,  0x8,  0x65, 0x2d, 0x66, 0x2d, 0x74,
    0x65, 0x72, 0x6d, 0x5,  0x8,  0x65, 0x2d, 0x66, 0x2d, 0x66, 0x61, 0x69,
    0x6c, 0x6,  0x3,  0x66, 0x2d, 0x31, 0x7,  0x3,  0x66, 0x2d, 0x32, 0x8,
    0x3,  0x66, 0x2d, 0x33, 0x9,  0x3,  0x66, 0x2d, 0x34, 0xa,  0x5,  0x66,
    0x2d, 0x61, 0x64, 0x64, 0xb,  0x7,  0x66, 0x2d, 0x6d, 0x75, 0x6c, 0x2d,
    0x32, 0xc,  0xa,  0x66, 0x2d, 0x63, 0x61, 0x6c, 0x6c, 0x2d, 0x69, 0x6e,
    0x64, 0xd,  0x7,  0x66, 0x2d, 0x65, 0x2d, 0x61, 0x64, 0x64, 0xe,  0x7,
    0x66, 0x2d, 0x65, 0x2d, 0x73, 0x75, 0x62, 0xf,  0x7,  0x66, 0x2d, 0x65,
    0x2d, 0x6d, 0x75, 0x6c, 0x10, 0x7,  0x66, 0x2d, 0x65, 0x2d, 0x64, 0x69,
    0x76, 0x4,  0xf,  0x2,  0x0,  0x5,  0x74, 0x79, 0x70, 0x65, 0x30, 0x1,
    0x5,  0x74, 0x79, 0x70, 0x65, 0x31, 0x5,  0xb,  0x2,  0x2,  0x3,  0x74,
    0x2d, 0x66, 0x3,  0x3,  0x74, 0x2d, 0x65, 0x6,  0x4,  0x1,  0x2,  0x1,
    0x6d, 0x7,  0xd,  0x2,  0x4,  0x4,  0x67, 0x2d, 0x6d, 0x69, 0x5,  0x4,
    0x67, 0x2d, 0x63, 0x66, 0xb,  0x16, 0x3,  0x3,  0x5,  0x74, 0x61, 0x67,
    0x2d, 0x31, 0x4,  0x5,  0x74, 0x61, 0x67, 0x2d, 0x32, 0x5,  0x5,  0x74,
    0x61, 0x67, 0x2d, 0x33};

std::vector<uint8_t> FibonacciWasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x6,  0x1,
    0x60, 0x1,  0x7f, 0x1,  0x7f, 0x3,  0x2,  0x1,  0x0,  0x7,  0x7,
    0x1,  0x3,  0x66, 0x69, 0x62, 0x0,  0x0,  0xa,  0x1f, 0x1,  0x1d,
    0x0,  0x20, 0x0,  0x41, 0x2,  0x48, 0x4,  0x40, 0x41, 0x1,  0xf,
    0xb,  0x20, 0x0,  0x41, 0x2,  0x6b, 0x10, 0x0,  0x20, 0x0,  0x41,
    0x1,  0x6b, 0x10, 0x0,  0x6a, 0xf,  0xb,  0x0,  0x15, 0x4,  0x6e,
    0x61, 0x6d, 0x65, 0x1,  0x6,  0x1,  0x0,  0x3,  0x66, 0x69, 0x62,
    0x2,  0x6,  0x1,  0x0,  0x1,  0x0,  0x1,  0x6e};

std::vector<char> ArgsVec = {
    'a', 'r', 'g', '1', '\0',
    // arg1
    'a', 'r', 'g', '2', '\0'
    // arg2
};
std::vector<char> EnvsVec = {
    'E', 'N', 'V', '1', '=', 'V', 'A', 'L', '1', '\0',
    // ENV1=VAL1
    'E', 'N', 'V', '2', '=', 'V', 'A', 'L', '2', '\0',
    // ENV2=VAL2
    'E', 'N', 'V', '3', '=', 'V', 'A', 'L', '3', '\0'
    // ENV3=VAL3
};
std::vector<char> PreopensVec = {
    'a', 'p', 'i', 'T', 'e', 's', 't', 'D', 'a', 't', 'a', '\0',
    // apiTestData
    'M', 'a', 'k', 'e', 'f', 'i', 'l', 'e', '\0',
    // Makefile
    'C', 'M', 'a', 'k', 'e', 'F', 'i', 'l', 'e', 's', '\0',
    // CMakeFiles
    's', 's', 'v', 'm', 'A', 'P', 'I', 'C', 'o', 'r', 'e', 'T', 'e', 's', 't',
    's', '\0',
    // wasmedgeAPICoreTests
    '.', ':', '.', '\0'
    // .:.
};
char *Args[] = {&ArgsVec[0], &ArgsVec[5]};
char *Envs[] = {&EnvsVec[0], &EnvsVec[10], &EnvsVec[20]};
char *Preopens[] = {&PreopensVec[0], &PreopensVec[12], &PreopensVec[21],
                    &PreopensVec[32], &PreopensVec[49]};
char TPath[] = "apiTestData/test.wasm";

void HexToFile(cxx20::span<const uint8_t> Wasm, const char *Path) {
  std::ofstream TFile(std::filesystem::u8path(Path), std::ios_base::binary);
  TFile.write(reinterpret_cast<const char *>(Wasm.data()),
              static_cast<std::streamsize>(Wasm.size()));
  TFile.close();
}

WasmEdge_Result ExternAdd(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 + Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternSub(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 - Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternMul(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 * Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternDiv(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 / Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternTerm(void *, const WasmEdge_CallingFrameContext *,
                           const WasmEdge_Value *, WasmEdge_Value *Out) {
  // {} -> {i32}
  Out[0] = WasmEdge_ValueGenI32(1234);
  return WasmEdge_Result_Terminate;
}

WasmEdge_Result ExternFail(void *, const WasmEdge_CallingFrameContext *,
                           const WasmEdge_Value *, WasmEdge_Value *Out) {
  // {} -> {i32}
  Out[0] = WasmEdge_ValueGenI32(5678);
  return WasmEdge_ResultGen(WasmEdge_ErrCategory_UserLevelError, 0x5678);
}

WasmEdge_Result ExternWrap(void *This, void *Data,
                           const WasmEdge_CallingFrameContext *MemCxt,
                           const WasmEdge_Value *In, const uint32_t,
                           WasmEdge_Value *Out, const uint32_t) {
  using HostFuncType =
      WasmEdge_Result(void *, const WasmEdge_CallingFrameContext *,
                      const WasmEdge_Value *, WasmEdge_Value *);
  HostFuncType *Func = reinterpret_cast<HostFuncType *>(This);
  return Func(Data, MemCxt, In, Out);
}

// Helper function to create import module instance with host functions
WasmEdge_ModuleInstanceContext *createExternModule
    [[maybe_unused]] (std::string_view Name, bool IsWrap = false) {
  // Create module instance
  WasmEdge_String HostName =
      WasmEdge_StringWrap(Name.data(), static_cast<uint32_t>(Name.length()));
  WasmEdge_ModuleInstanceContext *HostMod =
      WasmEdge_ModuleInstanceCreate(HostName);
  WasmEdge_ValType Param[2] = {WasmEdge_ValTypeGenExternRef(),
                               WasmEdge_ValTypeGenI32()},
                   Result[1] = {WasmEdge_ValTypeGenI32()};
  WasmEdge_FunctionTypeContext *HostFType =
      WasmEdge_FunctionTypeCreate(Param, 2, Result, 1);
  WasmEdge_FunctionInstanceContext *HostFunc = nullptr;

  // Add host function "func-add"
  HostName = WasmEdge_StringCreateByCString("func-add");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternAdd), nullptr, 0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternAdd, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  // Add host function "func-sub"
  HostName = WasmEdge_StringCreateByCString("func-sub");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternSub), nullptr, 0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternSub, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  // Add host function "func-mul"
  HostName = WasmEdge_StringCreateByCString("func-mul");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternMul), nullptr, 0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternMul, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  // Add host function "func-div"
  HostName = WasmEdge_StringCreateByCString("func-div");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternDiv), nullptr, 0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternDiv, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  WasmEdge_FunctionTypeDelete(HostFType);
  HostFType = WasmEdge_FunctionTypeCreate(nullptr, 0, Result, 1);

  // Add host function "func-term"
  HostName = WasmEdge_StringCreateByCString("func-term");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternTerm), nullptr,
        0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternTerm, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  // Add host function "func-fail"
  HostName = WasmEdge_StringCreateByCString("func-fail");
  if (IsWrap) {
    HostFunc = WasmEdge_FunctionInstanceCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternFail), nullptr,
        0);
  } else {
    HostFunc =
        WasmEdge_FunctionInstanceCreate(HostFType, ExternFail, nullptr, 0);
  }
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  WasmEdge_FunctionTypeDelete(HostFType);
  return HostMod;
}

// Helper function to load wasm file into AST module.
WasmEdge_ASTModuleContext *loadModule(const WasmEdge_ConfigureContext *Conf,
                                      const char *Path) {
  WasmEdge_ASTModuleContext *Mod = nullptr;
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(Conf);
  WasmEdge_LoaderParseFromFile(Loader, &Mod, Path);
  WasmEdge_LoaderDelete(Loader);
  return Mod;
}

// Helper function to validate wasm module.
bool validateModule(const WasmEdge_ConfigureContext *Conf,
                    const WasmEdge_ASTModuleContext *Mod) {
  WasmEdge_ValidatorContext *Validator = WasmEdge_ValidatorCreate(Conf);
  WasmEdge_Result Res = WasmEdge_ValidatorValidate(Validator, Mod);
  WasmEdge_ValidatorDelete(Validator);
  return WasmEdge_ResultOK(Res);
}

// Helper function to register AST module.
WasmEdge_ModuleInstanceContext *
registerModule(const WasmEdge_ConfigureContext *Conf,
               WasmEdge_StoreContext *Store,
               const WasmEdge_ASTModuleContext *Mod, std::string_view Name) {
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(Conf, nullptr);
  WasmEdge_String ModName =
      WasmEdge_StringWrap(Name.data(), static_cast<uint32_t>(Name.length()));
  WasmEdge_ModuleInstanceContext *ResMod = nullptr;
  WasmEdge_ExecutorRegister(ExecCxt, &ResMod, Store, Mod, ModName);
  WasmEdge_ExecutorDelete(ExecCxt);
  return ResMod;
}

// Helper function to register existing host module.
bool registerModule(const WasmEdge_ConfigureContext *Conf,
                    WasmEdge_StoreContext *Store,
                    WasmEdge_ModuleInstanceContext *ImpMod) {
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(Conf, nullptr);
  auto Res = WasmEdge_ExecutorRegisterImport(ExecCxt, Store, ImpMod);
  WasmEdge_ExecutorDelete(ExecCxt);
  return WasmEdge_ResultOK(Res);
}

// Helper function to instantiate module.
WasmEdge_ModuleInstanceContext *
instantiateModule(const WasmEdge_ConfigureContext *Conf,
                  WasmEdge_StoreContext *Store,
                  const WasmEdge_ASTModuleContext *Mod) {
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(Conf, nullptr);
  WasmEdge_ModuleInstanceContext *ResMod = nullptr;
  WasmEdge_ExecutorInstantiate(ExecCxt, &ResMod, Store, Mod);
  WasmEdge_ExecutorDelete(ExecCxt);
  return ResMod;
}

// Helper function to read file into a buffer.
bool readToVector(const char *Path, std::vector<uint8_t> &Buf) {
  std::ifstream F(Path, std::ios::binary | std::ios::ate);
  if (!F) {
    return false;
  }
  F.seekg(0, std::ios::end);
  Buf.resize(static_cast<uint32_t>(F.tellg()));
  F.seekg(0, std::ios::beg);
  if (!F.read(reinterpret_cast<char *>(Buf.data()),
              static_cast<uint32_t>(Buf.size()))) {
    return false;
  }
  F.close();
  return true;
}

// Helper function to check error code.
bool isErrMatch(WasmEdge_ErrCode Err, WasmEdge_Result Res) {
  return static_cast<uint32_t>(Err) == WasmEdge_ResultGetCode(Res);
}
bool isErrMatch(WasmEdge_ErrCategory ErrCate, uint32_t Code,
                WasmEdge_Result Res) {
  return ErrCate == WasmEdge_ResultGetCategory(Res) &&
         Code == WasmEdge_ResultGetCode(Res);
}

TEST(APICoreTest, Version) {
  EXPECT_EQ(std::string_view(WASMEDGE_VERSION),
            std::string_view(WasmEdge_VersionGet()));
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_MAJOR),
            WasmEdge_VersionGetMajor());
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_MINOR),
            WasmEdge_VersionGetMinor());
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_PATCH),
            WasmEdge_VersionGetPatch());
}

TEST(APICoreTest, Log) {
  WasmEdge_LogSetDebugLevel();
  EXPECT_TRUE(true);
  WasmEdge_LogSetErrorLevel();
  EXPECT_TRUE(true);
  WasmEdge_LogOff();
  EXPECT_TRUE(true);
}

TEST(APICoreTest, ValType) {
  WasmEdge_ValType VT;

  VT = WasmEdge_ValTypeGenI32();
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(VT));
  EXPECT_FALSE(WasmEdge_ValTypeIsRef(VT));

  VT = WasmEdge_ValTypeGenI64();
  EXPECT_TRUE(WasmEdge_ValTypeIsI64(VT));
  EXPECT_FALSE(WasmEdge_ValTypeIsRef(VT));

  VT = WasmEdge_ValTypeGenF32();
  EXPECT_TRUE(WasmEdge_ValTypeIsF32(VT));
  EXPECT_FALSE(WasmEdge_ValTypeIsRef(VT));

  VT = WasmEdge_ValTypeGenF64();
  EXPECT_TRUE(WasmEdge_ValTypeIsF64(VT));
  EXPECT_FALSE(WasmEdge_ValTypeIsRef(VT));

  VT = WasmEdge_ValTypeGenV128();
  EXPECT_TRUE(WasmEdge_ValTypeIsV128(VT));
  EXPECT_FALSE(WasmEdge_ValTypeIsRef(VT));

  VT = WasmEdge_ValTypeGenFuncRef();
  EXPECT_TRUE(WasmEdge_ValTypeIsFuncRef(VT));
  EXPECT_TRUE(WasmEdge_ValTypeIsRef(VT));
  EXPECT_TRUE(WasmEdge_ValTypeIsRefNull(VT));

  VT = WasmEdge_ValTypeGenExternRef();
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(VT));
  EXPECT_TRUE(WasmEdge_ValTypeIsRef(VT));
  EXPECT_TRUE(WasmEdge_ValTypeIsRefNull(VT));
}

TEST(APICoreTest, Value) {
  std::vector<uint32_t> Vec = {1U, 2U, 3U};
  WasmEdge_Value Val = WasmEdge_ValueGenI32(INT32_MAX);
  EXPECT_EQ(WasmEdge_ValueGetI32(Val), INT32_MAX);
  Val = WasmEdge_ValueGenI64(INT64_MAX);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), INT64_MAX);
  Val = WasmEdge_ValueGenF32(std::numeric_limits<float>::infinity());
  EXPECT_EQ(WasmEdge_ValueGetF32(Val), std::numeric_limits<float>::infinity());
  Val = WasmEdge_ValueGenF64(-std::numeric_limits<double>::infinity());
  EXPECT_EQ(WasmEdge_ValueGetF64(Val),
            -std::numeric_limits<double>::infinity());
#if defined(__x86_64__) || defined(__aarch64__)
  Val = WasmEdge_ValueGenV128(static_cast<int128_t>(INT64_MAX) * 2 + 1);
  EXPECT_EQ(WasmEdge_ValueGetV128(Val),
            static_cast<int128_t>(INT64_MAX) * 2 + 1);
#else
  int128_t V = {/* Low */ UINT64_MAX, /* High */ INT64_MAX};
  Val = WasmEdge_ValueGenV128(V);
  EXPECT_TRUE(0 == std::memcmp(&V, &Val, sizeof(V)));
#endif
  Val = WasmEdge_ValueGenFuncRef(nullptr);
  EXPECT_TRUE(WasmEdge_ValTypeIsFuncRef(Val.Type));
  EXPECT_EQ(WasmEdge_ValueGetFuncRef(Val), nullptr);
  Val = WasmEdge_ValueGenExternRef(&Vec);
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(Val.Type));
  EXPECT_EQ(
      static_cast<std::vector<uint32_t> *>(WasmEdge_ValueGetExternRef(Val))
          ->data()[1],
      2U);
}

TEST(APICoreTest, String) {
  // Test to delete nullptr.
  WasmEdge_String Str = {/* Length */ 0, /* Buf */ nullptr};
  WasmEdge_StringDelete(Str);
  EXPECT_TRUE(true);
  // Test strings.
  WasmEdge_String Str1 = WasmEdge_StringCreateByCString("test_string");
  WasmEdge_String Str2 = WasmEdge_StringCreateByCString("test_string");
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str1, Str2));
  const char CStr[] = "test_string_.....";
  WasmEdge_String Str3 = WasmEdge_StringCreateByBuffer(CStr, 11);
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str1, Str3));
  WasmEdge_String Str4 = WasmEdge_StringWrap(CStr, 11);
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str3, Str4));
  WasmEdge_String Str5 = WasmEdge_StringWrap(CStr, 13);
  EXPECT_FALSE(WasmEdge_StringIsEqual(Str3, Str5));
  WasmEdge_String Str6 = WasmEdge_StringCreateByCString(nullptr);
  EXPECT_EQ(Str6.Length, 0U);
  EXPECT_EQ(Str6.Buf, nullptr);
  WasmEdge_String Str7 = WasmEdge_StringCreateByBuffer(CStr, 0);
  EXPECT_EQ(Str7.Length, 0U);
  EXPECT_EQ(Str7.Buf, nullptr);
  WasmEdge_String Str8 = WasmEdge_StringCreateByBuffer(nullptr, 11);
  EXPECT_EQ(Str8.Length, 0U);
  EXPECT_EQ(Str8.Buf, nullptr);
  char Buf[256];
  EXPECT_EQ(WasmEdge_StringCopy(Str3, nullptr, 0), 0U);
  EXPECT_EQ(WasmEdge_StringCopy(Str3, Buf, 5), 5U);
  EXPECT_EQ(std::strncmp(Str3.Buf, Buf, 5), 0);
  EXPECT_EQ(WasmEdge_StringCopy(Str3, Buf, 256), 11U);
  WasmEdge_StringDelete(Str1);
  WasmEdge_StringDelete(Str2);
  WasmEdge_StringDelete(Str3);
}

TEST(APICoreTest, Bytes) {
  // Test to delete nullptr.
  WasmEdge_Bytes Buf = {/* Length */ 0, /* Buf */ nullptr};
  WasmEdge_BytesDelete(Buf);
  EXPECT_TRUE(true);
  // Test buffers.
  const uint8_t CBuf[] = {'t', 'e', 's', 't', '_', 'b', 'u', 'f'};
  WasmEdge_Bytes Buf1 = WasmEdge_BytesCreate(CBuf, 8U);
  EXPECT_EQ(Buf1.Length, 8U);
  EXPECT_NE(Buf1.Buf, nullptr);
  WasmEdge_Bytes Buf2 = WasmEdge_BytesCreate(nullptr, 0U);
  EXPECT_EQ(Buf2.Length, 0U);
  EXPECT_EQ(Buf2.Buf, nullptr);
  WasmEdge_Bytes Buf3 = WasmEdge_BytesCreate(CBuf, 0U);
  EXPECT_EQ(Buf3.Length, 0U);
  EXPECT_EQ(Buf3.Buf, nullptr);
  WasmEdge_Bytes Buf4 = WasmEdge_BytesCreate(nullptr, 8U);
  EXPECT_EQ(Buf4.Length, 0U);
  EXPECT_EQ(Buf4.Buf, nullptr);
  WasmEdge_Bytes Buf5 = WasmEdge_BytesWrap(CBuf, 8U);
  EXPECT_EQ(Buf5.Length, 8U);
  EXPECT_EQ(Buf5.Buf, CBuf);
  WasmEdge_BytesDelete(Buf1);
}

TEST(APICoreTest, Result) {
  WasmEdge_Result Res1 = WasmEdge_Result_Success;   // Success
  WasmEdge_Result Res2 = WasmEdge_Result_Terminate; // Terminated -> Success
  WasmEdge_Result Res3 = WasmEdge_Result_Fail;      // Failed
  EXPECT_TRUE(WasmEdge_ResultOK(Res1));
  EXPECT_TRUE(WasmEdge_ResultOK(Res2));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_RuntimeError, Res3));
  EXPECT_EQ(WasmEdge_ResultGetCode(Res1), 0x00U);
  EXPECT_EQ(WasmEdge_ResultGetCode(Res2), 0x01U);
  EXPECT_EQ(WasmEdge_ResultGetCode(Res3), 0x02U);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res1), nullptr);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res2), nullptr);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res3), nullptr);
}

TEST(APICoreTest, Configure) {
  WasmEdge_ConfigureContext *ConfNull = nullptr;
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  // Tests for proposals.
  WasmEdge_ConfigureAddProposal(ConfNull, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureAddProposal(ConfNull, WasmEdge_Proposal_Memory64);
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_Memory64);
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_SIMD));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(
      WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_Memory64));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_Memory64));
  WasmEdge_ConfigureRemoveProposal(Conf, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureRemoveProposal(ConfNull, WasmEdge_Proposal_SIMD);
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(
      WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_Memory64));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_Memory64));
  // Tests for host registrations.
  WasmEdge_ConfigureAddHostRegistration(ConfNull,
                                        WasmEdge_HostRegistration_Wasi);
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      ConfNull, WasmEdge_HostRegistration_Wasi));
  EXPECT_TRUE(WasmEdge_ConfigureHasHostRegistration(
      Conf, WasmEdge_HostRegistration_Wasi));
  WasmEdge_ConfigureRemoveHostRegistration(ConfNull,
                                           WasmEdge_HostRegistration_Wasi);
  WasmEdge_ConfigureRemoveHostRegistration(Conf,
                                           WasmEdge_HostRegistration_Wasi);
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      ConfNull, WasmEdge_HostRegistration_Wasi));
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      Conf, WasmEdge_HostRegistration_Wasi));
  // Tests for memory limits.
  WasmEdge_ConfigureSetMaxMemoryPage(ConfNull, 1234U);
  WasmEdge_ConfigureSetMaxMemoryPage(Conf, 1234U);
  EXPECT_NE(WasmEdge_ConfigureGetMaxMemoryPage(ConfNull), 1234U);
  EXPECT_EQ(WasmEdge_ConfigureGetMaxMemoryPage(Conf), 1234U);
  // Tests for force interpreter.
  WasmEdge_ConfigureSetForceInterpreter(ConfNull, true);
  EXPECT_EQ(WasmEdge_ConfigureIsForceInterpreter(Conf), false);
  WasmEdge_ConfigureSetForceInterpreter(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureIsForceInterpreter(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureIsForceInterpreter(Conf), true);
  // Tests for AOT compiler configurations.
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      ConfNull, WasmEdge_CompilerOptimizationLevel_Os);
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      Conf, WasmEdge_CompilerOptimizationLevel_Os);
  EXPECT_NE(WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfNull),
            WasmEdge_CompilerOptimizationLevel_Os);
  EXPECT_EQ(WasmEdge_ConfigureCompilerGetOptimizationLevel(Conf),
            WasmEdge_CompilerOptimizationLevel_Os);
  WasmEdge_ConfigureCompilerSetOutputFormat(
      ConfNull, WasmEdge_CompilerOutputFormat_Native);
  WasmEdge_ConfigureCompilerSetOutputFormat(
      Conf, WasmEdge_CompilerOutputFormat_Native);
  EXPECT_NE(WasmEdge_ConfigureCompilerGetOutputFormat(ConfNull),
            WasmEdge_CompilerOutputFormat_Native);
  EXPECT_EQ(WasmEdge_ConfigureCompilerGetOutputFormat(Conf),
            WasmEdge_CompilerOutputFormat_Native);
  WasmEdge_ConfigureCompilerSetDumpIR(ConfNull, true);
  WasmEdge_ConfigureCompilerSetDumpIR(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsDumpIR(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsDumpIR(Conf), true);
  WasmEdge_ConfigureCompilerSetGenericBinary(ConfNull, true);
  WasmEdge_ConfigureCompilerSetGenericBinary(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsGenericBinary(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsGenericBinary(Conf), true);
  WasmEdge_ConfigureCompilerSetInterruptible(ConfNull, true);
  WasmEdge_ConfigureCompilerSetInterruptible(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsInterruptible(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsInterruptible(Conf), true);
  // Tests for Statistics configurations.
  WasmEdge_ConfigureStatisticsSetInstructionCounting(ConfNull, true);
  WasmEdge_ConfigureStatisticsSetInstructionCounting(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureStatisticsIsInstructionCounting(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureStatisticsIsInstructionCounting(Conf), true);
  WasmEdge_ConfigureStatisticsSetCostMeasuring(ConfNull, true);
  WasmEdge_ConfigureStatisticsSetCostMeasuring(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureStatisticsIsCostMeasuring(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureStatisticsIsCostMeasuring(Conf), true);
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(ConfNull, true);
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureStatisticsIsTimeMeasuring(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureStatisticsIsTimeMeasuring(Conf), true);
  // Test to delete nullptr.
  WasmEdge_ConfigureDelete(ConfNull);
  EXPECT_TRUE(true);
  WasmEdge_ConfigureDelete(Conf);
  EXPECT_TRUE(true);
}

TEST(APICoreTest, FunctionType) {
  std::vector<WasmEdge_ValType> Param = {
      WasmEdge_ValTypeGenI32(),       WasmEdge_ValTypeGenI64(),
      WasmEdge_ValTypeGenExternRef(), WasmEdge_ValTypeGenV128(),
      WasmEdge_ValTypeGenF64(),       WasmEdge_ValTypeGenF32()};
  std::vector<WasmEdge_ValType> Result = {WasmEdge_ValTypeGenFuncRef(),
                                          WasmEdge_ValTypeGenExternRef(),
                                          WasmEdge_ValTypeGenV128()};
  WasmEdge_ValType Buf1[6], Buf2[2];
  WasmEdge_FunctionTypeContext *FType =
      WasmEdge_FunctionTypeCreate(&Param[0], 6, &Result[0], 3);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(FType), 6U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(FType), 3U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf1, 6), 6U);
  for (uint32_t I = 0; I < 6; I++) {
    EXPECT_TRUE(WasmEdge_ValTypeIsEqual(Param[I], Buf1[I]));
  }
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf2, 2), 6U);
  for (uint32_t I = 0; I < 2; I++) {
    EXPECT_TRUE(WasmEdge_ValTypeIsEqual(Param[I], Buf2[I]));
  }
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(nullptr, Buf1, 6), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf1, 6), 3U);
  for (uint32_t I = 0; I < 3; I++) {
    EXPECT_TRUE(WasmEdge_ValTypeIsEqual(Result[I], Buf1[I]));
  }
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf2, 2), 3U);
  for (uint32_t I = 0; I < 2; I++) {
    EXPECT_TRUE(WasmEdge_ValTypeIsEqual(Result[I], Buf2[I]));
  }
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(nullptr, Buf1, 6), 0U);
  WasmEdge_FunctionTypeDelete(FType);
  WasmEdge_FunctionTypeDelete(nullptr);

  FType = WasmEdge_FunctionTypeCreate(nullptr, 0, nullptr, 0);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf1, 6), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf1, 6), 0U);
  WasmEdge_FunctionTypeDelete(FType);
}

TEST(APICoreTest, TableType) {
  WasmEdge_Limit Lim1 = {/* HasMax */ true, /* Shared */ false, /* Min */ 10,
                         /* Max */ 20};
  WasmEdge_Limit Lim2 = {/* HasMax */ false, /* Shared */ false, /* Min */ 30,
                         /* Max */ 30};
  WasmEdge_TableTypeContext *TType =
      WasmEdge_TableTypeCreate(WasmEdge_ValTypeGenExternRef(), Lim1);
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(WasmEdge_TableTypeGetRefType(TType)));
  EXPECT_TRUE(WasmEdge_ValTypeIsFuncRef(WasmEdge_TableTypeGetRefType(nullptr)));
  EXPECT_TRUE(WasmEdge_LimitIsEqual(WasmEdge_TableTypeGetLimit(TType), Lim1));
  EXPECT_FALSE(
      WasmEdge_LimitIsEqual(WasmEdge_TableTypeGetLimit(nullptr), Lim1));
  WasmEdge_TableTypeDelete(TType);
  WasmEdge_TableTypeDelete(nullptr);
  TType = WasmEdge_TableTypeCreate(WasmEdge_ValTypeGenFuncRef(), Lim2);
  EXPECT_TRUE(WasmEdge_ValTypeIsFuncRef(WasmEdge_TableTypeGetRefType(TType)));
  EXPECT_TRUE(WasmEdge_LimitIsEqual(WasmEdge_TableTypeGetLimit(TType), Lim2));
  WasmEdge_TableTypeDelete(TType);
  WasmEdge_TableTypeDelete(nullptr);
}

TEST(APICoreTest, MemoryType) {
  WasmEdge_Limit Lim1 = {/* HasMax */ true, /* Shared */ false, /* Min */ 10,
                         /* Max */ 20};
  WasmEdge_Limit Lim2 = {/* HasMax */ false, /* Shared */ false, /* Min */ 30,
                         /* Max */ 30};
  WasmEdge_MemoryTypeContext *MType = WasmEdge_MemoryTypeCreate(Lim1);
  EXPECT_TRUE(WasmEdge_LimitIsEqual(WasmEdge_MemoryTypeGetLimit(MType), Lim1));
  EXPECT_FALSE(
      WasmEdge_LimitIsEqual(WasmEdge_MemoryTypeGetLimit(nullptr), Lim1));
  WasmEdge_MemoryTypeDelete(MType);
  WasmEdge_MemoryTypeDelete(nullptr);
  MType = WasmEdge_MemoryTypeCreate(Lim2);
  EXPECT_TRUE(WasmEdge_LimitIsEqual(WasmEdge_MemoryTypeGetLimit(MType), Lim2));
  WasmEdge_MemoryTypeDelete(nullptr);
  WasmEdge_MemoryTypeDelete(MType);
  WasmEdge_MemoryTypeDelete(nullptr);
}

TEST(APICoreTest, GlobalType) {
  WasmEdge_GlobalTypeContext *GType = WasmEdge_GlobalTypeCreate(
      WasmEdge_ValTypeGenV128(), WasmEdge_Mutability_Var);
  EXPECT_TRUE(WasmEdge_ValTypeIsV128(WasmEdge_GlobalTypeGetValType(GType)));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(WasmEdge_GlobalTypeGetValType(nullptr)));
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(GType), WasmEdge_Mutability_Var);
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(nullptr),
            WasmEdge_Mutability_Const);
  WasmEdge_GlobalTypeDelete(GType);
  WasmEdge_GlobalTypeDelete(nullptr);
}

TEST(APICoreTest, ImportType) {
  WasmEdge_ASTModuleContext *Mod = nullptr;
  const WasmEdge_ImportTypeContext *ImpTypes[20];
  WasmEdge_Limit Lim;
  WasmEdge_String Name;
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ExceptionHandling);
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(Conf);
  WasmEdge_ConfigureDelete(Conf);

  // Load AST module from buffer
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBytes(
      Loader, &Mod,
      WasmEdge_BytesWrap(ImportWasm.data(),
                         static_cast<uint32_t>(ImportWasm.size())))));
  EXPECT_NE(Mod, nullptr);

  // AST list imports
  EXPECT_EQ(WasmEdge_ASTModuleListImportsLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ASTModuleListImportsLength(Mod), 17U);
  EXPECT_EQ(WasmEdge_ASTModuleListImports(nullptr, ImpTypes, 20), 0U);
  EXPECT_EQ(WasmEdge_ASTModuleListImports(Mod, nullptr, 20), 17U);
  std::memset(ImpTypes, 0, sizeof(const WasmEdge_ImportTypeContext *) * 20);
  EXPECT_EQ(WasmEdge_ASTModuleListImports(Mod, ImpTypes, 4), 17U);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[0]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-add"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[1]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[1]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-sub"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[1]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[2]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[2]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-mul"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[2]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[3]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[3]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-div"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[3]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  std::memset(ImpTypes, 0, sizeof(const WasmEdge_ImportTypeContext *) * 20);
  EXPECT_EQ(WasmEdge_ASTModuleListImports(Mod, ImpTypes, 20), 17U);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[4]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[4]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-term"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[4]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[5]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[5]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-fail"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[5]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[6]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[6]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-i32"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[6]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[7]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[7]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-i64"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[7]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[8]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[8]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-f32"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[8]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[9]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[9]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-f64"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[9]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[10]),
            WasmEdge_ExternalType_Table);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[10]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tab-func"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[10]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[11]),
            WasmEdge_ExternalType_Table);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[11]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tab-ext"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[11]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[12]),
            WasmEdge_ExternalType_Memory);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[12]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "mem1"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[12]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[13]),
            WasmEdge_ExternalType_Memory);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[13]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "mem2"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[13]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[14]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[14]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag1"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[14]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[15]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[15]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag2"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[15]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[16]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[16]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag3"sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[16]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "dummy"sv);

  // Import type get external type
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(nullptr),
            WasmEdge_ExternalType_Function);
  EXPECT_EQ(WasmEdge_ImportTypeGetExternalType(ImpTypes[13]),
            WasmEdge_ExternalType_Memory);

  // Import type get module name
  Name = WasmEdge_ImportTypeGetModuleName(nullptr);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), ""sv);
  Name = WasmEdge_ImportTypeGetModuleName(ImpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "extern"sv);

  // Import type get external name
  Name = WasmEdge_ImportTypeGetExternalName(nullptr);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), ""sv);
  Name = WasmEdge_ImportTypeGetExternalName(ImpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-add"sv);

  // Import type get function type
  EXPECT_EQ(WasmEdge_ImportTypeGetFunctionType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetFunctionType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetFunctionType(nullptr, ImpTypes[4]), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetFunctionType(Mod, ImpTypes[8]), nullptr);
  EXPECT_NE(WasmEdge_ImportTypeGetFunctionType(Mod, ImpTypes[4]), nullptr);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(
                WasmEdge_ImportTypeGetFunctionType(Mod, ImpTypes[4])),
            0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(
                WasmEdge_ImportTypeGetFunctionType(Mod, ImpTypes[4])),
            1U);

  // Import type get table type
  EXPECT_EQ(WasmEdge_ImportTypeGetTableType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTableType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTableType(nullptr, ImpTypes[11]), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTableType(Mod, ImpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ImportTypeGetTableType(Mod, ImpTypes[11]), nullptr);
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(WasmEdge_TableTypeGetRefType(
      WasmEdge_ImportTypeGetTableType(Mod, ImpTypes[11]))));
  Lim = {/* HasMax */ true, /* Shared */ false, /* Min */ 10, /* Max */ 30};
  EXPECT_TRUE(WasmEdge_LimitIsEqual(
      WasmEdge_TableTypeGetLimit(
          WasmEdge_ImportTypeGetTableType(Mod, ImpTypes[11])),
      Lim));

  // Import type get memory type
  EXPECT_EQ(WasmEdge_ImportTypeGetMemoryType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetMemoryType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetMemoryType(nullptr, ImpTypes[13]), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetMemoryType(Mod, ImpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ImportTypeGetMemoryType(Mod, ImpTypes[13]), nullptr);
  Lim = {/* HasMax */ false, /* Shared */ false, /* Min */ 2, /* Max */ 2};
  EXPECT_TRUE(WasmEdge_LimitIsEqual(
      WasmEdge_MemoryTypeGetLimit(
          WasmEdge_ImportTypeGetMemoryType(Mod, ImpTypes[13])),
      Lim));

  // Import type get tag type
  EXPECT_EQ(WasmEdge_ImportTypeGetTagType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTagType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTagType(nullptr, ImpTypes[15]), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetTagType(Mod, ImpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ImportTypeGetTagType(Mod, ImpTypes[15]), nullptr);
  EXPECT_EQ(WasmEdge_TagTypeGetFunctionType(nullptr), nullptr);
  EXPECT_NE(WasmEdge_TagTypeGetFunctionType(
                WasmEdge_ImportTypeGetTagType(Mod, ImpTypes[15])),
            nullptr);
  EXPECT_EQ(
      WasmEdge_FunctionTypeGetParametersLength(WasmEdge_TagTypeGetFunctionType(
          WasmEdge_ImportTypeGetTagType(Mod, ImpTypes[15]))),
      2U);
  EXPECT_EQ(
      WasmEdge_FunctionTypeGetReturnsLength(WasmEdge_TagTypeGetFunctionType(
          WasmEdge_ImportTypeGetTagType(Mod, ImpTypes[15]))),
      0U);

  // Import type get global type
  EXPECT_EQ(WasmEdge_ImportTypeGetGlobalType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetGlobalType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetGlobalType(nullptr, ImpTypes[7]), nullptr);
  EXPECT_EQ(WasmEdge_ImportTypeGetGlobalType(Mod, ImpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ImportTypeGetGlobalType(Mod, ImpTypes[7]), nullptr);
  EXPECT_TRUE(WasmEdge_ValTypeIsI64(WasmEdge_GlobalTypeGetValType(
      WasmEdge_ImportTypeGetGlobalType(Mod, ImpTypes[7]))));
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(
                WasmEdge_ImportTypeGetGlobalType(Mod, ImpTypes[7])),
            WasmEdge_Mutability_Const);

  WasmEdge_LoaderDelete(Loader);
  WasmEdge_ASTModuleDelete(Mod);
}

TEST(APICoreTest, ExportType) {
  WasmEdge_ASTModuleContext *Mod = nullptr;
  const WasmEdge_ExportTypeContext *ExpTypes[20];
  WasmEdge_Limit Lim;
  WasmEdge_String Name;
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ExceptionHandling);
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(Conf);
  WasmEdge_ConfigureDelete(Conf);

  // Load AST module from buffer
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBytes(
      Loader, &Mod,
      WasmEdge_BytesWrap(ImportWasm.data(),
                         static_cast<uint32_t>(ImportWasm.size())))));
  EXPECT_NE(Mod, nullptr);

  // AST list exports
  EXPECT_EQ(WasmEdge_ASTModuleListExportsLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ASTModuleListExportsLength(Mod), 19U);
  EXPECT_EQ(WasmEdge_ASTModuleListExports(nullptr, ExpTypes, 20), 0U);
  EXPECT_EQ(WasmEdge_ASTModuleListExports(Mod, nullptr, 20), 19U);
  std::memset(ExpTypes, 0, sizeof(const WasmEdge_ExportTypeContext *) * 20);
  EXPECT_EQ(WasmEdge_ASTModuleListExports(Mod, ExpTypes, 4), 19U);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[0]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-1"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[1]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[1]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-2"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[2]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[2]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-3"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[3]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[3]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-4"sv);
  std::memset(ExpTypes, 0, sizeof(const WasmEdge_ExportTypeContext *) * 20);
  EXPECT_EQ(WasmEdge_ASTModuleListExports(Mod, ExpTypes, 20), 19U);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[4]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[4]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-add"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[5]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[5]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-mul-2"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[6]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[6]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-call-indirect"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[7]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[7]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-host-add"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[8]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[8]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-host-sub"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[9]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[9]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-host-mul"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[10]),
            WasmEdge_ExternalType_Function);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[10]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-host-div"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[11]),
            WasmEdge_ExternalType_Table);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[11]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tab-func"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[12]),
            WasmEdge_ExternalType_Table);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[12]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tab-ext"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[13]),
            WasmEdge_ExternalType_Memory);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[13]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "mem"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[14]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[14]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag-1"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[15]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[15]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag-2"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[16]),
            WasmEdge_ExternalType_Tag);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[16]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "tag-3"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[17]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[17]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-mut-i32"sv);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[18]),
            WasmEdge_ExternalType_Global);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[18]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "glob-const-f32"sv);

  // Export type get external type
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(nullptr),
            WasmEdge_ExternalType_Function);
  EXPECT_EQ(WasmEdge_ExportTypeGetExternalType(ExpTypes[18]),
            WasmEdge_ExternalType_Global);

  // Export type get external name
  Name = WasmEdge_ExportTypeGetExternalName(nullptr);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), ""sv);
  Name = WasmEdge_ExportTypeGetExternalName(ExpTypes[0]);
  EXPECT_EQ(std::string_view(Name.Buf, Name.Length), "func-1"sv);

  // Export type get function type
  EXPECT_EQ(WasmEdge_ExportTypeGetFunctionType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetFunctionType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetFunctionType(nullptr, ExpTypes[4]), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetFunctionType(Mod, ExpTypes[15]), nullptr);
  EXPECT_NE(WasmEdge_ExportTypeGetFunctionType(Mod, ExpTypes[4]), nullptr);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(
                WasmEdge_ExportTypeGetFunctionType(Mod, ExpTypes[4])),
            2U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(
                WasmEdge_ExportTypeGetFunctionType(Mod, ExpTypes[4])),
            1U);

  // Export type get table type
  EXPECT_EQ(WasmEdge_ExportTypeGetTableType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTableType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTableType(nullptr, ExpTypes[12]), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTableType(Mod, ExpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ExportTypeGetTableType(Mod, ExpTypes[12]), nullptr);
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(WasmEdge_TableTypeGetRefType(
      WasmEdge_ExportTypeGetTableType(Mod, ExpTypes[12]))));
  Lim = {/* HasMax */ false, /* Shared */ false, /* Min */ 10, /* Max */ 10};
  EXPECT_TRUE(WasmEdge_LimitIsEqual(
      WasmEdge_TableTypeGetLimit(
          WasmEdge_ExportTypeGetTableType(Mod, ExpTypes[12])),
      Lim));

  // Export type get memory type
  EXPECT_EQ(WasmEdge_ExportTypeGetMemoryType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetMemoryType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetMemoryType(nullptr, ExpTypes[13]), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetMemoryType(Mod, ExpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ExportTypeGetMemoryType(Mod, ExpTypes[13]), nullptr);
  Lim = {/* HasMax */ true, /* Shared */ false, /* Min */ 1, /* Max */ 3};
  EXPECT_TRUE(WasmEdge_LimitIsEqual(
      WasmEdge_MemoryTypeGetLimit(
          WasmEdge_ExportTypeGetMemoryType(Mod, ExpTypes[13])),
      Lim));

  // Export type get tag type
  EXPECT_EQ(WasmEdge_ExportTypeGetTagType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTagType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTagType(nullptr, ExpTypes[14]), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetTagType(Mod, ExpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ExportTypeGetTagType(Mod, ExpTypes[14]), nullptr);
  EXPECT_EQ(WasmEdge_TagTypeGetFunctionType(nullptr), nullptr);
  EXPECT_NE(WasmEdge_TagTypeGetFunctionType(
                WasmEdge_ExportTypeGetTagType(Mod, ExpTypes[14])),
            nullptr);
  EXPECT_EQ(
      WasmEdge_FunctionTypeGetParametersLength(WasmEdge_TagTypeGetFunctionType(
          WasmEdge_ExportTypeGetTagType(Mod, ExpTypes[14]))),
      4U);
  EXPECT_EQ(
      WasmEdge_FunctionTypeGetReturnsLength(WasmEdge_TagTypeGetFunctionType(
          WasmEdge_ExportTypeGetTagType(Mod, ExpTypes[14]))),
      0U);

  // Export type get global type
  EXPECT_EQ(WasmEdge_ExportTypeGetGlobalType(nullptr, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetGlobalType(Mod, nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetGlobalType(nullptr, ExpTypes[18]), nullptr);
  EXPECT_EQ(WasmEdge_ExportTypeGetGlobalType(Mod, ExpTypes[0]), nullptr);
  EXPECT_NE(WasmEdge_ExportTypeGetGlobalType(Mod, ExpTypes[18]), nullptr);
  EXPECT_TRUE(WasmEdge_ValTypeIsF32(WasmEdge_GlobalTypeGetValType(
      WasmEdge_ExportTypeGetGlobalType(Mod, ExpTypes[18]))));
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(
                WasmEdge_ExportTypeGetGlobalType(Mod, ExpTypes[18])),
            WasmEdge_Mutability_Const);

  WasmEdge_LoaderDelete(Loader);
  WasmEdge_ASTModuleDelete(Mod);
}

#ifdef WASMEDGE_USE_LLVM
TEST(APICoreTest, Compiler) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  std::ifstream OutFile;
  uint8_t Buf[4];
  uint8_t WASMMagic[] = {0x00, 0x61, 0x73, 0x6D};

  // Compiler creation and deletion
  WasmEdge_CompilerContext *Compiler = WasmEdge_CompilerCreate(nullptr);
  EXPECT_NE(Compiler, nullptr);
  WasmEdge_CompilerDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_CompilerDelete(Compiler);
  EXPECT_TRUE(true);
  Compiler = WasmEdge_CompilerCreate(Conf);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // Compile file for universal WASM output format
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_CompilerCompile(Compiler, TPath, "test_aot.wasm")));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_CompilerCompile(
      Compiler, "../spec/testSuites/core/binary/binary.55.wasm",
      "success_aot.wasm")));
  // File not found
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_IllegalPath,
                         WasmEdge_CompilerCompile(Compiler, "not_exist.wasm",
                                                  "not_exist_aot.wasm")));
  // Parse failed
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_UnexpectedEnd,
      WasmEdge_CompilerCompile(Compiler,
                               "../spec/testSuites/core/binary/binary.4.wasm",
                               "parse_error_aot.wasm")));
  WasmEdge_CompilerDelete(Compiler);
  // Check the header of the output files.
  OutFile.open("test_aot.wasm", std::ios::binary);
  EXPECT_TRUE(OutFile.read(reinterpret_cast<char *>(Buf), 4));
  OutFile.close();
  EXPECT_TRUE(std::equal(WASMMagic, WASMMagic + 4, Buf));
  OutFile.open("success_aot.wasm", std::ios::binary);
  EXPECT_TRUE(OutFile.read(reinterpret_cast<char *>(Buf), 4));
  OutFile.close();
  EXPECT_TRUE(std::equal(WASMMagic, WASMMagic + 4, Buf));

  // Compile file for shared library output format
  WasmEdge_ConfigureCompilerSetOutputFormat(
      Conf, WasmEdge_CompilerOutputFormat_Native);
  Compiler = WasmEdge_CompilerCreate(Conf);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_CompilerCompile(
      Compiler, TPath, "test_aot" WASMEDGE_LIB_EXTENSION)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_CompilerCompile(
      Compiler, "../spec/testSuites/core/binary/binary.55.wasm",
      "success_aot" WASMEDGE_LIB_EXTENSION)));
  // Check the header of the output files.
  OutFile.open("test_aot" WASMEDGE_LIB_EXTENSION, std::ios::binary);
  EXPECT_TRUE(OutFile.read(reinterpret_cast<char *>(Buf), 4));
  OutFile.close();
  EXPECT_FALSE(std::equal(WASMMagic, WASMMagic + 4, Buf));
  OutFile.open("success_aot" WASMEDGE_LIB_EXTENSION, std::ios::binary);
  EXPECT_TRUE(OutFile.read(reinterpret_cast<char *>(Buf), 4));
  OutFile.close();
  EXPECT_FALSE(std::equal(WASMMagic, WASMMagic + 4, Buf));

  // Compile file for shared library output format from buffer
  std::error_code EC;
  auto TPathFS = std::filesystem::u8path(TPath);
  size_t FileSize = std::filesystem::file_size(TPathFS, EC);
  EXPECT_FALSE(EC);
  std::ifstream Fin(TPathFS, std::ios::in | std::ios::binary);
  EXPECT_TRUE(Fin);
  std::vector<uint8_t> Data(FileSize);
  size_t Index = 0;
  while (FileSize > 0) {
    const uint32_t BlockSize = static_cast<uint32_t>(
        std::min<size_t>(FileSize, std::numeric_limits<uint32_t>::max()));
    Fin.read(reinterpret_cast<char *>(Data.data()) + Index, BlockSize);
    const uint32_t ReadCount = static_cast<uint32_t>(Fin.gcount());
    EXPECT_TRUE(ReadCount == BlockSize);
    Index += static_cast<size_t>(BlockSize);
    FileSize -= static_cast<size_t>(BlockSize);
  }
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_CompilerCompileFromBuffer(
      Compiler, Data.data(), Data.size(), "test_aot" WASMEDGE_LIB_EXTENSION)));
  // Check the header of the output files.
  OutFile.open("test_aot" WASMEDGE_LIB_EXTENSION, std::ios::binary);
  EXPECT_TRUE(OutFile.read(reinterpret_cast<char *>(Buf), 4));
  OutFile.close();
  EXPECT_FALSE(std::equal(WASMMagic, WASMMagic + 4, Buf));

  // Compile file for universal WASM output format repeatedly
  WasmEdge_CompilerDelete(Compiler);
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      Conf, WasmEdge_CompilerOptimizationLevel_O0);
  WasmEdge_ConfigureCompilerSetOutputFormat(Conf,
                                            WasmEdge_CompilerOutputFormat_Wasm);
  Compiler = WasmEdge_CompilerCreate(Conf);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_CompilerCompileFromBuffer(
      Compiler, FibonacciWasm.data(), FibonacciWasm.size(), "fib_aot1.wasm")));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_CompilerCompile(Compiler, "fib_aot1.wasm", "fib_aot2.wasm")));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_CompilerCompile(Compiler, "fib_aot2.wasm", "fib_aot3.wasm")));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_CompilerCompile(Compiler, "fib_aot3.wasm", "fib_aot4.wasm")));
  // Test the output universal WASM
  WasmEdge_Value P[1], R[1];
  P[0] = WasmEdge_ValueGenI32(20);
  R[0] = WasmEdge_ValueGenI32(0);
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(Conf, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMRunWasmFromFile(VM, "fib_aot4.wasm", FuncName, P, 1, R, 1);
  WasmEdge_VMDelete(VM);
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 10946);
  // Test the force-interpreter mode of the universal WASM
  R[0] = WasmEdge_ValueGenI32(0);
  WasmEdge_ConfigureSetForceInterpreter(Conf, true);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMRunWasmFromFile(VM, "fib_aot4.wasm", FuncName, P, 1, R, 1);
  WasmEdge_VMDelete(VM);
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 10946);
  WasmEdge_StringDelete(FuncName);

  WasmEdge_CompilerDelete(Compiler);
  WasmEdge_ConfigureDelete(Conf);
}
#endif

TEST(APICoreTest, Loader) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ASTModuleContext *Mod = nullptr;
  WasmEdge_ASTModuleContext **ModPtr = &Mod;

  // Loader creation and deletion
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(nullptr);
  EXPECT_NE(Loader, nullptr);
  WasmEdge_LoaderDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_LoaderDelete(Loader);
  EXPECT_TRUE(true);
  Loader = WasmEdge_LoaderCreate(Conf);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // Parse from file
  Mod = nullptr;
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(Loader, ModPtr, TPath)));
  EXPECT_NE(Mod, nullptr);
  WasmEdge_ASTModuleDelete(Mod);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_LoaderParseFromFile(nullptr, ModPtr, TPath)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_LoaderParseFromFile(Loader, nullptr, TPath)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_IllegalPath,
                         WasmEdge_LoaderParseFromFile(Loader, ModPtr, "file")));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_LoaderParseFromFile(nullptr, nullptr, TPath)));

  // Parse from buffer
  std::vector<uint8_t> Buf;
  EXPECT_TRUE(readToVector(TPath, Buf));
  Mod = nullptr;
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBuffer(
      Loader, ModPtr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_NE(Mod, nullptr);
  WasmEdge_ASTModuleDelete(Mod);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_LoaderParseFromBuffer(nullptr, ModPtr, Buf.data(),
                                     static_cast<uint32_t>(Buf.size()))));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_LoaderParseFromBuffer(Loader, nullptr, Buf.data(),
                                     static_cast<uint32_t>(Buf.size()))));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_UnexpectedEnd,
                 WasmEdge_LoaderParseFromBuffer(Loader, ModPtr, nullptr, 0)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_LoaderParseFromBuffer(nullptr, nullptr, Buf.data(),
                                     static_cast<uint32_t>(Buf.size()))));
#ifdef WASMEDGE_USE_LLVM
  // Failed case to parse from buffer with AOT compiled WASM
  EXPECT_TRUE(readToVector("test_aot" WASMEDGE_LIB_EXTENSION, Buf));
  Mod = nullptr;
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_MalformedMagic,
      WasmEdge_LoaderParseFromBuffer(Loader, ModPtr, Buf.data(),
                                     static_cast<uint32_t>(Buf.size()))));
#endif

  // AST module deletion
  WasmEdge_ASTModuleDelete(nullptr);
  EXPECT_TRUE(true);

  WasmEdge_LoaderDelete(Loader);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(APICoreTest, Validator) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();

  // Validator creation and deletion
  WasmEdge_ValidatorContext *Validator = WasmEdge_ValidatorCreate(nullptr);
  EXPECT_NE(Validator, nullptr);
  WasmEdge_ValidatorDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ValidatorDelete(Validator);
  EXPECT_TRUE(true);
  Validator = WasmEdge_ValidatorCreate(Conf);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // Load and parse file
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf, TPath);
  EXPECT_NE(Mod, nullptr);

  // Validation
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_ValidatorValidate(Validator, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_ValidatorValidate(nullptr, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_ValidatorValidate(Validator, nullptr)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_ValidatorValidate(nullptr, nullptr)));

  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ValidatorDelete(Validator);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(APICoreTest, ExecutorWithStatistics) {
  // Create contexts
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();

  // Enable Statistics
  WasmEdge_ConfigureStatisticsSetInstructionCounting(Conf, true);
  WasmEdge_ConfigureStatisticsSetCostMeasuring(Conf, true);
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(Conf, true);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // Load and validate file
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf, TPath);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  // Statistics creation and deletion
  WasmEdge_StatisticsContext *Stat = WasmEdge_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);
  WasmEdge_StatisticsDelete(Stat);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsDelete(nullptr);
  EXPECT_TRUE(true);
  Stat = WasmEdge_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);

  // Statistics set cost table
  std::vector<uint64_t> CostTable(512, 20ULL);
  WasmEdge_StatisticsSetCostTable(nullptr, &CostTable[0], 512);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostTable(Stat, nullptr, 0);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostTable(Stat, &CostTable[0], 512);
  EXPECT_TRUE(true);

  // Statistics set cost limit
  WasmEdge_StatisticsSetCostLimit(Stat, 100000000000000ULL);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostLimit(nullptr, 1ULL);
  EXPECT_TRUE(true);

  // Executor creation and deletion
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(nullptr, nullptr);
  EXPECT_NE(ExecCxt, nullptr);
  WasmEdge_ExecutorDelete(ExecCxt);
  EXPECT_TRUE(true);
  ExecCxt = WasmEdge_ExecutorCreate(Conf, nullptr);
  EXPECT_NE(ExecCxt, nullptr);
  WasmEdge_ExecutorDelete(ExecCxt);
  EXPECT_TRUE(true);
  ExecCxt = WasmEdge_ExecutorCreate(nullptr, Stat);
  EXPECT_NE(ExecCxt, nullptr);
  WasmEdge_ExecutorDelete(ExecCxt);
  EXPECT_TRUE(true);
  ExecCxt = WasmEdge_ExecutorCreate(Conf, Stat);
  EXPECT_NE(ExecCxt, nullptr);
  WasmEdge_ExecutorDelete(nullptr);
  EXPECT_TRUE(true);

  // Register import object
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule("extern");
  EXPECT_NE(HostMod, nullptr);
  WasmEdge_ModuleInstanceContext *HostModWrap =
      createExternModule("extern-wrap", true);
  EXPECT_NE(HostModWrap, nullptr);
  WasmEdge_ModuleInstanceContext *HostMod2 = createExternModule("extern");
  EXPECT_NE(HostMod2, nullptr);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorRegisterImport(nullptr, Store, HostMod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorRegisterImport(ExecCxt, nullptr, HostMod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorRegisterImport(ExecCxt, Store, nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorRegisterImport(ExecCxt, Store, HostMod)));
  // Name conflict
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_ModuleNameConflict,
                 WasmEdge_ExecutorRegisterImport(ExecCxt, Store, HostMod2)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorRegisterImport(ExecCxt, Store, HostModWrap)));
  WasmEdge_ModuleInstanceDelete(HostMod2);

  // Register wasm module
  WasmEdge_String ModName = WasmEdge_StringCreateByCString("module");
  WasmEdge_String ModName2 = WasmEdge_StringCreateByCString("extern");
  WasmEdge_ModuleInstanceContext *ModRegCxt = nullptr;
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_ExecutorRegister(nullptr, &ModRegCxt, Store, Mod, ModName)));
  EXPECT_EQ(ModRegCxt, nullptr);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_ExecutorRegister(ExecCxt, nullptr, Store, Mod, ModName)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_ExecutorRegister(ExecCxt, &ModRegCxt, nullptr, Mod, ModName)));
  EXPECT_EQ(ModRegCxt, nullptr);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_ExecutorRegister(ExecCxt, &ModRegCxt, Store, nullptr, ModName)));
  EXPECT_EQ(ModRegCxt, nullptr);
  // Name conflict
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_ModuleNameConflict,
      WasmEdge_ExecutorRegister(ExecCxt, &ModRegCxt, Store, Mod, ModName2)));
  EXPECT_EQ(ModRegCxt, nullptr);
  // Hasn't validated yet
  WasmEdge_ASTModuleContext *ModNotValid = loadModule(Conf, TPath);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_NotValidated,
                         WasmEdge_ExecutorRegister(ExecCxt, &ModRegCxt, Store,
                                                   ModNotValid, ModName)));
  EXPECT_EQ(ModRegCxt, nullptr);
  WasmEdge_ASTModuleDelete(ModNotValid);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorRegister(ExecCxt, &ModRegCxt, Store, Mod, ModName)));
  EXPECT_NE(ModRegCxt, nullptr);
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);

  // Instantiate wasm module
  WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorInstantiate(nullptr, &ModCxt, Store, Mod)));
  EXPECT_EQ(ModCxt, nullptr);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorInstantiate(ExecCxt, nullptr, Store, Mod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, nullptr, Mod)));
  EXPECT_EQ(ModCxt, nullptr);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, Store, nullptr)));
  EXPECT_EQ(ModCxt, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, Store, Mod)));
  EXPECT_NE(ModCxt, nullptr);
  WasmEdge_ASTModuleDelete(Mod);

  // Invoke functions
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("func-mul-2");
  WasmEdge_FunctionInstanceContext *FuncCxt =
      WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_Value P[2], R[2];
  P[0] = WasmEdge_ValueGenI32(123);
  P[1] = WasmEdge_ValueGenI32(456);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorInvoke(nullptr, FuncCxt, P, 2, R, 2)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_ExecutorInvoke(ExecCxt, nullptr, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, nullptr, 0)));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, nullptr, 1)));

  // Invoke functions call to host functions
  // Get table and set external reference
  uint32_t TestValue;
  WasmEdge_String TabName = WasmEdge_StringCreateByCString("tab-ext");
  WasmEdge_TableInstanceContext *TabCxt =
      WasmEdge_ModuleInstanceFindTable(ModCxt, TabName);
  EXPECT_NE(TabCxt, nullptr);
  WasmEdge_StringDelete(TabName);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 1)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 2)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 3)));
  // Call add: (777) + (223)
  FuncName = WasmEdge_StringCreateByCString("func-host-add");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  P[0] = WasmEdge_ValueGenI32(223);
  TestValue = 777;
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 1, R, 1)));
  EXPECT_EQ(1000, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Call sub: (123) - (456)
  FuncName = WasmEdge_StringCreateByCString("func-host-sub");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  P[0] = WasmEdge_ValueGenI32(456);
  TestValue = 123;
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 1, R, 1)));
  EXPECT_EQ(-333, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Call mul: (-30) * (-66)
  FuncName = WasmEdge_StringCreateByCString("func-host-mul");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  P[0] = WasmEdge_ValueGenI32(-66);
  TestValue = static_cast<uint32_t>(-30);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 1, R, 1)));
  EXPECT_EQ(1980, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Call div: (-9999) / (1234)
  FuncName = WasmEdge_StringCreateByCString("func-host-div");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  P[0] = WasmEdge_ValueGenI32(1234);
  TestValue = static_cast<uint32_t>(-9999);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 1, R, 1)));
  EXPECT_EQ(-8, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));

  // Invoke functions of registered module
  FuncName = WasmEdge_StringCreateByCString("func-add");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostMod, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  TestValue = 5000;
  P[0] = WasmEdge_ValueGenExternRef(&TestValue);
  P[1] = WasmEdge_ValueGenI32(1500);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, R, 1)));
  EXPECT_EQ(6500, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));

  // Invoke host function to terminate or fail execution
  FuncName = WasmEdge_StringCreateByCString("func-term");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostMod, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 1)));
  FuncName = WasmEdge_StringCreateByCString("func-fail");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostMod, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCategory_UserLevelError, 0x5678U,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 1)));

  // Invoke host function with binding to functions
  FuncName = WasmEdge_StringCreateByCString("func-sub");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostModWrap, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  TestValue = 1234;
  P[0] = WasmEdge_ValueGenExternRef(&TestValue);
  P[1] = WasmEdge_ValueGenI32(1500);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, P, 2, R, 1)));
  EXPECT_EQ(-266, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  FuncName = WasmEdge_StringCreateByCString("func-term");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostModWrap, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 1)));
  FuncName = WasmEdge_StringCreateByCString("func-fail");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(HostModWrap, FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCategory_UserLevelError, 0x5678U,
                 WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 1)));

  // Invoke independent host functions
  // host function "func-add": {externref, i32} -> {i32}
  WasmEdge_ValType Result[1] = {WasmEdge_ValTypeGenI32()};
  WasmEdge_FunctionTypeContext *FuncType =
      WasmEdge_FunctionTypeCreate(nullptr, 0, Result, 1);
  FuncCxt = WasmEdge_FunctionInstanceCreate(FuncType, ExternTerm, nullptr, 0);
  WasmEdge_FunctionTypeDelete(FuncType);
  EXPECT_NE(FuncCxt, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, R, 1)));
  WasmEdge_FunctionInstanceDelete(FuncCxt);
  EXPECT_TRUE(true);

  // Statistics get instruction count
  EXPECT_GT(WasmEdge_StatisticsGetInstrCount(Stat), 0ULL);
  EXPECT_EQ(WasmEdge_StatisticsGetInstrCount(nullptr), 0ULL);

  // Statistics get instruction per second
  EXPECT_GT(WasmEdge_StatisticsGetInstrPerSecond(Stat), 0.0);
  EXPECT_EQ(WasmEdge_StatisticsGetInstrPerSecond(nullptr), 0.0);

  // Statistics get total cost
  EXPECT_GT(WasmEdge_StatisticsGetTotalCost(Stat), 0ULL);
  EXPECT_EQ(WasmEdge_StatisticsGetTotalCost(nullptr), 0ULL);

  // Statistics clear
  WasmEdge_StatisticsClear(Stat);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsClear(nullptr);
  EXPECT_TRUE(true);

  WasmEdge_ConfigureDelete(Conf);
  WasmEdge_ExecutorDelete(ExecCxt);
  WasmEdge_StoreDelete(Store);
  WasmEdge_StatisticsDelete(Stat);
  WasmEdge_ModuleInstanceDelete(ModCxt);
  WasmEdge_ModuleInstanceDelete(ModRegCxt);
  WasmEdge_ModuleInstanceDelete(HostMod);
  WasmEdge_ModuleInstanceDelete(HostModWrap);
}

TEST(APICoreTest, Store) {
  // Create contexts
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();
  WasmEdge_String Names[15], ErrName, ModName[3];
  ModName[0] = WasmEdge_StringCreateByCString("module");
  ModName[1] = WasmEdge_StringCreateByCString("extern");
  ModName[2] = WasmEdge_StringCreateByCString("no-such-module");
  ErrName = WasmEdge_StringCreateByCString("invalid-instance-name");

  // Store list module before instantiation
  EXPECT_EQ(WasmEdge_StoreListModuleLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListModuleLength(Store), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, nullptr, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, Names, 15), 0U);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // Register host module and instantiate wasm module
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule("extern");
  EXPECT_NE(HostMod, nullptr);
  EXPECT_TRUE(registerModule(Conf, Store, HostMod));
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf, TPath);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));
  WasmEdge_ModuleInstanceContext *ModRegCxt =
      registerModule(Conf, Store, Mod, "module");
  EXPECT_NE(ModRegCxt, nullptr);
  WasmEdge_ModuleInstanceContext *ModCxt = instantiateModule(Conf, Store, Mod);
  EXPECT_NE(ModCxt, nullptr);
  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ConfigureDelete(Conf);

  // Store find module
  EXPECT_EQ(WasmEdge_StoreFindModule(Store, ModName[0]), ModRegCxt);
  EXPECT_EQ(WasmEdge_StoreFindModule(Store, ModName[1]), HostMod);
  EXPECT_EQ(WasmEdge_StoreFindModule(nullptr, ModName[1]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindModule(Store, ModName[2]), nullptr);

  // Store list module
  EXPECT_EQ(WasmEdge_StoreListModuleLength(Store), 2U);
  EXPECT_EQ(WasmEdge_StoreListModuleLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, nullptr, 15), 2U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, Names, 1), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "extern"sv);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, Names, 15), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "extern"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "module"sv);

  // Module instance get module name
  Names[0] = WasmEdge_ModuleInstanceGetModuleName(nullptr);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), ""sv);
  Names[0] = WasmEdge_ModuleInstanceGetModuleName(ModCxt);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), ""sv);
  Names[0] = WasmEdge_ModuleInstanceGetModuleName(ModRegCxt);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "module"sv);
  Names[0] = WasmEdge_ModuleInstanceGetModuleName(HostMod);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "extern"sv);

  // Module instance list function exports
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunctionLength(ModCxt), 11U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunctionLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModCxt, nullptr, 15), 11U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModCxt, Names, 4), 11U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "func-1"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "func-2"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length), "func-3"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "func-4"sv);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModCxt, Names, 15), 11U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "func-1"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "func-2"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length), "func-3"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "func-4"sv);
  EXPECT_EQ(std::string_view(Names[4].Buf, Names[4].Length), "func-add"sv);
  EXPECT_EQ(std::string_view(Names[5].Buf, Names[5].Length),
            "func-call-indirect"sv);
  EXPECT_EQ(std::string_view(Names[6].Buf, Names[6].Length), "func-host-add"sv);
  EXPECT_EQ(std::string_view(Names[7].Buf, Names[7].Length), "func-host-div"sv);
  EXPECT_EQ(std::string_view(Names[8].Buf, Names[8].Length), "func-host-mul"sv);
  EXPECT_EQ(std::string_view(Names[9].Buf, Names[9].Length), "func-host-sub"sv);
  EXPECT_EQ(std::string_view(Names[10].Buf, Names[10].Length), "func-mul-2"sv);

  // Module instance find function
  EXPECT_NE(WasmEdge_ModuleInstanceFindFunction(ModCxt, Names[7]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindFunction(nullptr, Names[7]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindFunction(ModCxt, ErrName), nullptr);

  // Module instance list table exports
  EXPECT_EQ(WasmEdge_ModuleInstanceListTableLength(ModCxt), 2U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListTableLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListTable(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListTable(ModCxt, nullptr, 15), 2U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListTable(ModCxt, Names, 1), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "tab-ext"sv);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListTable(ModCxt, Names, 15), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "tab-ext"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "tab-func"sv);

  // Module instance find table
  EXPECT_NE(WasmEdge_ModuleInstanceFindTable(ModCxt, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindTable(nullptr, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindTable(ModCxt, ErrName), nullptr);

  // Module instance list memory exports
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemoryLength(ModCxt), 1U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemoryLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemory(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemory(ModCxt, nullptr, 15), 1U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemory(ModCxt, Names, 0), 1U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListMemory(ModCxt, Names, 15), 1U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "mem"sv);

  // Module instance find memory
  EXPECT_NE(WasmEdge_ModuleInstanceFindMemory(ModCxt, Names[0]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindMemory(nullptr, Names[0]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindMemory(ModCxt, ErrName), nullptr);

  // Module instance list global exports
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobalLength(ModCxt), 2U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobalLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobal(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobal(ModCxt, nullptr, 15), 2U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobal(ModCxt, Names, 1), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length),
            "glob-const-f32"sv);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_ModuleInstanceListGlobal(ModCxt, Names, 15), 2U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length),
            "glob-const-f32"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "glob-mut-i32"sv);

  // Module instance find global
  EXPECT_NE(WasmEdge_ModuleInstanceFindGlobal(ModCxt, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindGlobal(nullptr, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceFindGlobal(ModCxt, ErrName), nullptr);

  // Delete module
  WasmEdge_ModuleInstanceDelete(HostMod);
  WasmEdge_ModuleInstanceDelete(ModCxt);
  WasmEdge_ModuleInstanceDelete(ModRegCxt);

  // Test store after module deletion
  EXPECT_EQ(WasmEdge_StoreListModuleLength(Store), 0U);

  // Store deletion
  WasmEdge_StoreDelete(nullptr);
  EXPECT_TRUE(true);

  WasmEdge_StringDelete(ModName[0]);
  WasmEdge_StringDelete(ModName[1]);
  WasmEdge_StringDelete(ModName[2]);
  WasmEdge_StringDelete(ErrName);
  WasmEdge_StoreDelete(Store);
}

TEST(APICoreTest, Instance) {
  WasmEdge_ValType VType;
  WasmEdge_Value Val, TmpVal;

  // WasmEdge_ModuleInstanceContext related APIs tested in `Store` and
  // `ModuleInstance` test case.

  // Function instance
  WasmEdge_FunctionInstanceContext *FuncCxt;
  WasmEdge_ValType Param[2], Result[1];
  Param[0] = WasmEdge_ValTypeGenExternRef();
  Param[1] = WasmEdge_ValTypeGenI32();
  Result[0] = WasmEdge_ValTypeGenI32();
  WasmEdge_FunctionTypeContext *FuncType =
      WasmEdge_FunctionTypeCreate(Param, 2, Result, 1);

  // Function instance creation
  // host function "func-add": {externref, i32} -> {i32}
  FuncCxt = WasmEdge_FunctionInstanceCreate(nullptr, ExternAdd, nullptr, 0);
  EXPECT_EQ(FuncCxt, nullptr);
  FuncCxt = WasmEdge_FunctionInstanceCreate(FuncType, nullptr, nullptr, 0);
  EXPECT_EQ(FuncCxt, nullptr);
  FuncCxt = WasmEdge_FunctionInstanceCreate(FuncType, ExternAdd, nullptr, 0);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_FunctionInstanceDelete(FuncCxt);
  EXPECT_TRUE(true);

  // Function instance create binding
  // host function for binding "func-add-binding": {externref, i32} -> {i32}
  FuncCxt = WasmEdge_FunctionInstanceCreateBinding(
      nullptr, ExternWrap, reinterpret_cast<void *>(ExternAdd), nullptr, 0);
  EXPECT_EQ(FuncCxt, nullptr);
  FuncCxt = WasmEdge_FunctionInstanceCreateBinding(
      FuncType, nullptr, reinterpret_cast<void *>(ExternAdd), nullptr, 0);
  EXPECT_EQ(FuncCxt, nullptr);
  FuncCxt = WasmEdge_FunctionInstanceCreateBinding(
      FuncType, ExternWrap, reinterpret_cast<void *>(ExternAdd), nullptr, 0);
  EXPECT_NE(FuncCxt, nullptr);
  WasmEdge_FunctionTypeDelete(FuncType);

  // Function instance get function type
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(
                WasmEdge_FunctionInstanceGetFunctionType(FuncCxt)),
            2U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(
                WasmEdge_FunctionInstanceGetFunctionType(FuncCxt)),
            1U);
  EXPECT_NE(WasmEdge_FunctionInstanceGetFunctionType(FuncCxt), nullptr);
  EXPECT_EQ(WasmEdge_FunctionInstanceGetFunctionType(nullptr), nullptr);

  // Function instance deletion
  WasmEdge_FunctionInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_FunctionInstanceDelete(FuncCxt);
  EXPECT_TRUE(true);

  // Table instance
  WasmEdge_TableInstanceContext *TabCxt;
  WasmEdge_TableTypeContext *TabType;

  // Table instance creation
  TabCxt = WasmEdge_TableInstanceCreate(nullptr);
  EXPECT_EQ(TabCxt, nullptr);
  TabType = WasmEdge_TableTypeCreate(
      WasmEdge_ValTypeGenExternRef(),
      WasmEdge_Limit{/* HasMax */ false, /* Shared */ false, /* Min */ 10,
                     /* Max */ 10});
  TabCxt = WasmEdge_TableInstanceCreate(TabType);
  WasmEdge_TableTypeDelete(TabType);
  EXPECT_NE(TabCxt, nullptr);
  WasmEdge_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);
  TabType = WasmEdge_TableTypeCreate(
      WasmEdge_ValTypeGenExternRef(),
      WasmEdge_Limit{/* HasMax */ true, /* Shared */ false, /* Min */ 10,
                     /* Max */ 20});
  TabCxt = WasmEdge_TableInstanceCreate(TabType);
  WasmEdge_TableTypeDelete(TabType);
  EXPECT_NE(TabCxt, nullptr);

  // Table instance get table type
  EXPECT_TRUE(WasmEdge_ValTypeIsExternRef(WasmEdge_TableTypeGetRefType(
      WasmEdge_TableInstanceGetTableType(TabCxt))));
  EXPECT_EQ(WasmEdge_TableInstanceGetTableType(nullptr), nullptr);

  // Table instance set data
  Val = WasmEdge_ValueGenExternRef(&TabCxt);
  TmpVal = WasmEdge_ValueGenFuncRef(nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 5)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_RefTypeMismatch,
                         WasmEdge_TableInstanceSetData(TabCxt, TmpVal, 6)));
  TmpVal = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_RefTypeMismatch,
                         WasmEdge_TableInstanceSetData(TabCxt, TmpVal, 7)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_TableInstanceSetData(nullptr, Val, 5)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_TableOutOfBounds,
                         WasmEdge_TableInstanceSetData(TabCxt, Val, 15)));

  // Table instance get data
  Val = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(TabCxt, &Val, 5)));
  EXPECT_EQ(reinterpret_cast<WasmEdge_TableInstanceContext **>(
                WasmEdge_ValueGetExternRef(Val)),
            &TabCxt);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_TableInstanceGetData(nullptr, &Val, 5)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_TableOutOfBounds,
                         WasmEdge_TableInstanceGetData(TabCxt, &Val, 15)));

  // Table instance get size and grow
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 10U);
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(nullptr), 0U);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_TableInstanceGrow(nullptr, 8)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 18U);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_TableOutOfBounds,
                         WasmEdge_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 18U);
  Val = WasmEdge_ValueGenExternRef(&TabCxt);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 15)));
  Val = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(TabCxt, &Val, 15)));
  EXPECT_EQ(reinterpret_cast<WasmEdge_TableInstanceContext **>(
                WasmEdge_ValueGetExternRef(Val)),
            &TabCxt);

  // Table instance deletion
  WasmEdge_TableInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);

  // Table instance create with init
  VType = WasmEdge_ValTypeGenExternRef();
  // TODO: Forcibly change to non-nullable. Refine this after providing the
  // corresponding API.
  VType.Data[2] = WasmEdge_TypeCode_Ref;
  TabType = WasmEdge_TableTypeCreate(
      VType, WasmEdge_Limit{/* HasMax */ true, /* Shared */ false, /* Min */ 10,
                            /* Max */ 10});
  TabCxt = WasmEdge_TableInstanceCreate(TabType);
  EXPECT_EQ(TabCxt, nullptr);
  Val = WasmEdge_ValueGenFuncRef(nullptr);
  TabCxt = WasmEdge_TableInstanceCreateWithInit(nullptr, Val);
  EXPECT_EQ(TabCxt, nullptr);
  TabCxt = WasmEdge_TableInstanceCreateWithInit(TabType, Val);
  EXPECT_EQ(TabCxt, nullptr);
  Val = WasmEdge_ValueGenExternRef(nullptr);
  TabCxt = WasmEdge_TableInstanceCreateWithInit(TabType, Val);
  EXPECT_EQ(TabCxt, nullptr);
  Val = WasmEdge_ValueGenExternRef(&TabType);
  TabCxt = WasmEdge_TableInstanceCreateWithInit(TabType, Val);
  EXPECT_NE(TabCxt, nullptr);
  WasmEdge_TableTypeDelete(TabType);

  // Table instance set data with non-nullable reference
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 5)));
  Val = WasmEdge_ValueGenExternRef(nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_NonNullRequired,
                         WasmEdge_TableInstanceSetData(TabCxt, Val, 5)));
  WasmEdge_TableInstanceDelete(TabCxt);

  // Memory instance
  WasmEdge_MemoryInstanceContext *MemCxt;
  WasmEdge_MemoryTypeContext *MemType;

  // Memory instance creation
  MemCxt = WasmEdge_MemoryInstanceCreate(nullptr);
  EXPECT_EQ(MemCxt, nullptr);
  MemType = WasmEdge_MemoryTypeCreate(WasmEdge_Limit{
      /* HasMax */ false, /* Shared */ false, /* Min */ 1, /* Max */ 1});
  MemCxt = WasmEdge_MemoryInstanceCreate(MemType);
  WasmEdge_MemoryTypeDelete(MemType);
  EXPECT_NE(MemCxt, nullptr);
  WasmEdge_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);
  MemType = WasmEdge_MemoryTypeCreate(WasmEdge_Limit{
      /* HasMax */ true, /* Shared */ false, /* Min */ 1, /* Max */ 3});
  MemCxt = WasmEdge_MemoryInstanceCreate(MemType);
  WasmEdge_MemoryTypeDelete(MemType);
  EXPECT_NE(MemCxt, nullptr);

  // Memory instance get memory type
  EXPECT_NE(WasmEdge_MemoryInstanceGetMemoryType(MemCxt), nullptr);
  EXPECT_EQ(WasmEdge_MemoryInstanceGetMemoryType(nullptr), nullptr);

  // Memory instance set data
  std::vector<uint8_t> DataSet = {'t', 'e', 's', 't', ' ',
                                  'd', 'a', 't', 'a', '\n'};
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 10)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_MemoryInstanceSetData(nullptr, DataSet.data(), 100, 10)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_MemoryInstanceSetData(MemCxt, nullptr, 100, 0)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_MemoryInstanceSetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 0)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_MemoryOutOfBounds,
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 65536, 10)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_MemoryOutOfBounds,
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 65530, 10)));

  // Memory instance get data
  std::vector<uint8_t> DataGet;
  DataGet.resize(10);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 10)));
  EXPECT_EQ(DataGet, DataSet);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_MemoryInstanceGetData(nullptr, DataGet.data(), 100, 10)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_MemoryInstanceGetData(MemCxt, nullptr, 100, 0)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_MemoryInstanceGetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 0)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_MemoryOutOfBounds,
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 65536, 10)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_MemoryOutOfBounds,
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 65530, 10)));

  // Memory instance get pointer
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(nullptr, 100, 10));
  EXPECT_NE(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 100, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 65536, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 65530, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(nullptr, 100, 10));
  EXPECT_NE(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 100, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 65536, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 65530, 10));
  EXPECT_TRUE(std::equal(DataSet.cbegin(), DataSet.cend(),
                         WasmEdge_MemoryInstanceGetPointer(MemCxt, 100, 10)));
  EXPECT_TRUE(
      std::equal(DataSet.cbegin(), DataSet.cend(),
                 WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 100, 10)));

  // Memory instance get size and grow
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 1U);
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(nullptr), 0U);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_MemoryInstanceGrowPage(nullptr, 1)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_MemoryInstanceGrowPage(MemCxt, 1)));
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_MemoryOutOfBounds,
                         WasmEdge_MemoryInstanceGrowPage(MemCxt, 2)));
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 70000, 10)));
  DataGet.clear();
  DataGet.resize(10);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 70000, 10)));
  EXPECT_EQ(DataGet, DataSet);

  // Memory instance deletion
  WasmEdge_MemoryInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);

  // Global instance
  WasmEdge_GlobalInstanceContext *GlobCCxt, *GlobVCxt;
  WasmEdge_GlobalTypeContext *GlobCType, *GlobVType;

  // Global instance creation
  GlobVCxt = WasmEdge_GlobalInstanceCreate(nullptr, WasmEdge_ValueGenI32(0));
  EXPECT_EQ(GlobVCxt, nullptr);
  GlobVType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenF32(),
                                        WasmEdge_Mutability_Var);
  GlobVCxt = WasmEdge_GlobalInstanceCreate(GlobVType, WasmEdge_ValueGenI32(0));
  WasmEdge_GlobalTypeDelete(GlobVType);
  EXPECT_EQ(GlobVCxt, nullptr);
  GlobCType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenI64(),
                                        WasmEdge_Mutability_Const);
  GlobVType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenI64(),
                                        WasmEdge_Mutability_Var);
  GlobCCxt = WasmEdge_GlobalInstanceCreate(GlobCType,
                                           WasmEdge_ValueGenI64(55555555555LL));
  GlobVCxt = WasmEdge_GlobalInstanceCreate(GlobVType,
                                           WasmEdge_ValueGenI64(66666666666LL));
  WasmEdge_GlobalTypeDelete(GlobCType);
  WasmEdge_GlobalTypeDelete(GlobVType);
  EXPECT_NE(GlobCCxt, nullptr);
  EXPECT_NE(GlobVCxt, nullptr);

  // Global instance get global type
  EXPECT_TRUE(WasmEdge_ValTypeIsI64(WasmEdge_GlobalTypeGetValType(
      WasmEdge_GlobalInstanceGetGlobalType(GlobCCxt))));
  EXPECT_TRUE(WasmEdge_ValTypeIsI64(WasmEdge_GlobalTypeGetValType(
      WasmEdge_GlobalInstanceGetGlobalType(GlobVCxt))));
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(
                WasmEdge_GlobalInstanceGetGlobalType(GlobCCxt)),
            WasmEdge_Mutability_Const);
  EXPECT_EQ(WasmEdge_GlobalTypeGetMutability(
                WasmEdge_GlobalInstanceGetGlobalType(GlobVCxt)),
            WasmEdge_Mutability_Var);
  EXPECT_EQ(WasmEdge_GlobalInstanceGetGlobalType(nullptr), nullptr);

  // Global instance get value
  Val = WasmEdge_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 55555555555LL);
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 66666666666LL);
  Val = WasmEdge_GlobalInstanceGetValue(nullptr);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 0LL);

  // Global instance set value
  Val = WasmEdge_ValueGenI64(77777777777LL);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_SetValueToConst,
                         WasmEdge_GlobalInstanceSetValue(GlobCCxt, Val)));
  Val = WasmEdge_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 55555555555LL);
  Val = WasmEdge_ValueGenI64(88888888888LL);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val)));
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 88888888888LL);
  Val = WasmEdge_ValueGenF32(12.345f);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_SetValueErrorType,
                         WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val)));
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 88888888888LL);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_GlobalInstanceSetValue(nullptr, Val)));

  // Global instance deletion
  WasmEdge_GlobalInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_GlobalInstanceDelete(GlobCCxt);
  EXPECT_TRUE(true);
  WasmEdge_GlobalInstanceDelete(GlobVCxt);
  EXPECT_TRUE(true);

  // Global instance with non-nullable reference
  VType = WasmEdge_ValTypeGenExternRef();
  // TODO: Forcibly change to non-nullable. Refine this after providing the
  // corresponding API.
  VType.Data[2] = WasmEdge_TypeCode_Ref;
  GlobVType = WasmEdge_GlobalTypeCreate(VType, WasmEdge_Mutability_Var);
  Val = WasmEdge_ValueGenFuncRef(nullptr);
  GlobVCxt = WasmEdge_GlobalInstanceCreate(GlobVType, Val);
  EXPECT_EQ(GlobVCxt, nullptr);
  Val = WasmEdge_ValueGenExternRef(nullptr);
  GlobVCxt = WasmEdge_GlobalInstanceCreate(GlobVType, Val);
  EXPECT_EQ(GlobVCxt, nullptr);
  Val = WasmEdge_ValueGenExternRef(&GlobVType);
  GlobVCxt = WasmEdge_GlobalInstanceCreate(GlobVType, Val);
  EXPECT_NE(GlobVCxt, nullptr);
  WasmEdge_GlobalTypeDelete(GlobVType);

  // Global instance set value with non-nullable reference
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val)));
  Val = WasmEdge_ValueGenExternRef(nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_NonNullRequired,
                         WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val)));
  Val = WasmEdge_ValueGenFuncRef(nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_RefTypeMismatch,
                         WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val)));
  WasmEdge_GlobalInstanceDelete(GlobVCxt);
}

TEST(APICoreTest, ModuleInstance) {
  WasmEdge_String HostName;
  WasmEdge_ConfigureContext *Conf = nullptr;
  WasmEdge_VMContext *VM = nullptr;
  WasmEdge_ModuleInstanceContext *HostMod = nullptr;
  WasmEdge_FunctionTypeContext *HostFType = nullptr;
  WasmEdge_TableTypeContext *HostTType = nullptr;
  WasmEdge_MemoryTypeContext *HostMType = nullptr;
  WasmEdge_GlobalTypeContext *HostGType = nullptr;
  WasmEdge_FunctionInstanceContext *HostFunc = nullptr;
  WasmEdge_TableInstanceContext *HostTable = nullptr;
  WasmEdge_MemoryInstanceContext *HostMemory = nullptr;
  WasmEdge_GlobalInstanceContext *HostGlobal = nullptr;
  auto HostFinalizer = [](void *Data) {
    fmt::print("Data address: {}\n"sv, Data);
  };
  WasmEdge_ValType Param[2], Result[1];

  // Create module instance with name ""
  HostMod = WasmEdge_ModuleInstanceCreate({/* Length */ 0, /* Buf */ nullptr});
  EXPECT_NE(HostMod, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(HostMod), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(nullptr), nullptr);
  WasmEdge_ModuleInstanceDelete(HostMod);

  // Create module instance with empty host data and finalizer
  HostMod = WasmEdge_ModuleInstanceCreateWithData(
      {/* Length */ 0, /* Buf */ nullptr}, nullptr, nullptr);
  EXPECT_NE(HostMod, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(HostMod), nullptr);
  WasmEdge_ModuleInstanceDelete(HostMod);

  // Create module instance with host data and finalizer
  HostMod = WasmEdge_ModuleInstanceCreateWithData(
      {/* Length */ 0, /* Buf */ nullptr}, nullptr, HostFinalizer);
  EXPECT_NE(HostMod, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(HostMod), nullptr);
  WasmEdge_ModuleInstanceDelete(HostMod);
  HostMod = WasmEdge_ModuleInstanceCreateWithData(
      {/* Length */ 0, /* Buf */ nullptr}, &HostName, nullptr);
  EXPECT_NE(HostMod, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(HostMod), &HostName);
  WasmEdge_ModuleInstanceDelete(HostMod);
  HostMod = WasmEdge_ModuleInstanceCreateWithData(
      {/* Length */ 0, /* Buf */ nullptr}, &HostName, HostFinalizer);
  EXPECT_NE(HostMod, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceGetHostData(HostMod), &HostName);
  WasmEdge_ModuleInstanceDelete(HostMod);

  // Create module instance with name "extern"
  HostName = WasmEdge_StringCreateByCString("extern");
  HostMod = WasmEdge_ModuleInstanceCreate(HostName);
  EXPECT_NE(HostMod, nullptr);
  EXPECT_TRUE(WasmEdge_StringIsEqual(
      HostName, WasmEdge_ModuleInstanceGetModuleName(HostMod)));
  WasmEdge_StringDelete(HostName);

  // Add host function "func-add": {externref, i32} -> {i32}
  Param[0] = WasmEdge_ValTypeGenExternRef();
  Param[1] = WasmEdge_ValTypeGenI32();
  Result[0] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, Result, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternAdd, nullptr, 0);
  EXPECT_NE(HostFunc, nullptr);
  HostName = WasmEdge_StringCreateByCString("func-add");
  WasmEdge_ModuleInstanceAddFunction(nullptr, HostName, HostFunc);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  EXPECT_TRUE(true);
  WasmEdge_FunctionTypeDelete(HostFType);
  WasmEdge_StringDelete(HostName);

  // Add host table "table"
  WasmEdge_Limit TabLimit = {/* HasMax */ true, /* Shared */ false,
                             /* Min */ 10, /* Max */ 20};
  HostTType = WasmEdge_TableTypeCreate(WasmEdge_ValTypeGenFuncRef(), TabLimit);
  HostTable = WasmEdge_TableInstanceCreate(HostTType);
  EXPECT_NE(HostTable, nullptr);
  HostName = WasmEdge_StringCreateByCString("table");
  WasmEdge_ModuleInstanceAddTable(nullptr, HostName, HostTable);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddTable(HostMod, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddTable(HostMod, HostName, HostTable);
  EXPECT_TRUE(true);
  WasmEdge_TableTypeDelete(HostTType);
  WasmEdge_StringDelete(HostName);

  // Add host memory "memory"
  WasmEdge_Limit MemLimit = {/* HasMax */ true, /* Shared */ false, /* Min */ 1,
                             /* Max */ 2};
  HostMType = WasmEdge_MemoryTypeCreate(MemLimit);
  HostMemory = WasmEdge_MemoryInstanceCreate(HostMType);
  EXPECT_NE(HostMemory, nullptr);
  HostName = WasmEdge_StringCreateByCString("memory");
  WasmEdge_ModuleInstanceAddMemory(nullptr, HostName, HostMemory);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddMemory(HostMod, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddMemory(HostMod, HostName, HostMemory);
  EXPECT_TRUE(true);
  WasmEdge_MemoryTypeDelete(HostMType);
  WasmEdge_StringDelete(HostName);

  // Add host global "global_i32": const 666
  HostGType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenI32(),
                                        WasmEdge_Mutability_Const);
  HostGlobal =
      WasmEdge_GlobalInstanceCreate(HostGType, WasmEdge_ValueGenI32(666));
  EXPECT_NE(HostGlobal, nullptr);
  HostName = WasmEdge_StringCreateByCString("global_i32");
  WasmEdge_ModuleInstanceAddGlobal(nullptr, HostName, HostGlobal);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
  EXPECT_TRUE(true);
  WasmEdge_GlobalTypeDelete(HostGType);
  WasmEdge_StringDelete(HostName);

  // Delete module instance.
  WasmEdge_ModuleInstanceDelete(HostMod);

  // Create WASI.
  HostMod = WasmEdge_ModuleInstanceCreateWASI(Args, 2, Envs, 3, Preopens, 5);
  EXPECT_NE(HostMod, nullptr);
  WasmEdge_ModuleInstanceDelete(HostMod);
  HostMod =
      WasmEdge_ModuleInstanceCreateWASI(nullptr, 0, nullptr, 0, nullptr, 0);
  EXPECT_NE(HostMod, nullptr);
  WasmEdge_ModuleInstanceDelete(HostMod);
  HostMod = WasmEdge_ModuleInstanceCreateWASI(Args, 0, Envs, 3, Preopens, 5);
  EXPECT_NE(HostMod, nullptr);
  // Check the Native Handler
  {
    // STDIN
#if WASMEDGE_OS_WINDOWS
    const uint64_t StdIn = reinterpret_cast<uint64_t>(
        WasmEdge::winapi::GetStdHandle(WasmEdge::winapi::STD_INPUT_HANDLE_));
#else
    const uint64_t StdIn = STDIN_FILENO;
#endif
    uint64_t NativeHandler = 100;
    auto RetStatus =
        WasmEdge_ModuleInstanceWASIGetNativeHandler(HostMod, 0, &NativeHandler);
    EXPECT_EQ(RetStatus, 0);
    EXPECT_EQ(NativeHandler, StdIn);
  }
  {
    // STDOUT
#if WASMEDGE_OS_WINDOWS
    const uint64_t StdOut = reinterpret_cast<uint64_t>(
        WasmEdge::winapi::GetStdHandle(WasmEdge::winapi::STD_OUTPUT_HANDLE_));
#else
    const uint64_t StdOut = STDOUT_FILENO;
#endif
    uint64_t NativeHandler = 100;
    auto RetStatus =
        WasmEdge_ModuleInstanceWASIGetNativeHandler(HostMod, 1, &NativeHandler);
    EXPECT_EQ(RetStatus, 0);
    EXPECT_EQ(NativeHandler, StdOut);
  }
  {
    // STDERR
#if WASMEDGE_OS_WINDOWS
    const uint64_t StdErr = reinterpret_cast<uint64_t>(
        WasmEdge::winapi::GetStdHandle(WasmEdge::winapi::STD_ERROR_HANDLE_));
#else
    const uint64_t StdErr = STDERR_FILENO;
#endif
    uint64_t NativeHandler = 100;
    auto RetStatus =
        WasmEdge_ModuleInstanceWASIGetNativeHandler(HostMod, 2, &NativeHandler);
    EXPECT_EQ(RetStatus, 0);
    EXPECT_EQ(NativeHandler, StdErr);
  }
  {
    // non-existed fd
    uint64_t NativeHandler = 100;
    auto RetStatus = WasmEdge_ModuleInstanceWASIGetNativeHandler(
        HostMod, 9527, &NativeHandler);
    EXPECT_EQ(RetStatus, 2);
    EXPECT_EQ(NativeHandler, 100);
  }
  // Get WASI exit code.
  EXPECT_EQ(WasmEdge_ModuleInstanceWASIGetExitCode(HostMod), EXIT_SUCCESS);
  EXPECT_EQ(WasmEdge_ModuleInstanceWASIGetExitCode(nullptr), EXIT_FAILURE);
  WasmEdge_ModuleInstanceDelete(HostMod);

  // Initialize WASI in VM.
  Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_ConfigureDelete(Conf);
  HostMod =
      WasmEdge_VMGetImportModuleContext(VM, WasmEdge_HostRegistration_Wasi);
  EXPECT_NE(HostMod, nullptr);
  WasmEdge_ModuleInstanceInitWASI(nullptr, Args, 2, Envs, 3, Preopens, 5);
  EXPECT_TRUE(true);
  WasmEdge_ModuleInstanceInitWASI(HostMod, Args, 2, Envs, 3, Preopens, 5);
  EXPECT_TRUE(true);
  // Get WASI exit code.
  EXPECT_EQ(WasmEdge_ModuleInstanceWASIGetExitCode(HostMod), EXIT_SUCCESS);
  EXPECT_EQ(WasmEdge_ModuleInstanceWASIGetExitCode(nullptr), EXIT_FAILURE);
  WasmEdge_VMDelete(VM);
}

TEST(APICoreTest, Async) {
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(nullptr, nullptr);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();
  WasmEdge_ExecutorContext *Exec = WasmEdge_ExecutorCreate(nullptr, nullptr);
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule("extern");
  WasmEdge_VMRegisterModuleFromImport(VM, HostMod);
  WasmEdge_String ModName, ModName2, FuncName, FuncName2;
  WasmEdge_Value P[10], R[10];
  WasmEdge_Async *Async = nullptr;

  ModName = WasmEdge_StringCreateByCString("reg-wasm-buffer");
  ModName2 = WasmEdge_StringCreateByCString("reg-wasm-error");
  FuncName = WasmEdge_StringCreateByCString("func-mul-2");
  FuncName2 = WasmEdge_StringCreateByCString("func-mul-3");
  P[0] = WasmEdge_ValueGenI32(123);
  P[1] = WasmEdge_ValueGenI32(456);

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // WASM from file
  std::vector<uint8_t> Buf;
  EXPECT_TRUE(readToVector(TPath, Buf));

  // Load and validate to wasm AST
  WasmEdge_ASTModuleContext *Mod = loadModule(nullptr, TPath);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(nullptr, Mod));

  // Async deletion
  WasmEdge_AsyncDelete(nullptr);
  EXPECT_TRUE(true);

  // Async wait and waitfor
  WasmEdge_AsyncWait(nullptr);
  EXPECT_TRUE(true);
  EXPECT_FALSE(WasmEdge_AsyncWaitFor(nullptr, 1234));

  // Async cancel
  WasmEdge_AsyncCancel(nullptr);
  EXPECT_TRUE(true);

  // Async get returns length
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(nullptr), 0);

  // Async run from file
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  // Success case
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  WasmEdge_AsyncDelete(Async);
  // VM nullptr case
  Async = WasmEdge_VMAsyncRunWasmFromFile(nullptr, TPath, FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // File path not found case
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, "no_file", FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_IllegalPath, WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName2, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromFile(VM, TPath, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  // Async run from buffer
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  // Success case
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  WasmEdge_AsyncDelete(Async);
  // VM nullptr case
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Buffer nullptr case
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(VM, nullptr, 0, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_UnexpectedEnd,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName2, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  // Async run from AST module
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  // Success case
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  WasmEdge_AsyncDelete(Async);
  // VM nullptr case
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(nullptr, Mod, FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // AST module nullptr case
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, nullptr, FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName2, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_VMAsyncRunWasmFromASTModule(VM, Mod, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  // Async VM execute
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  WasmEdge_VMRegisterModuleFromImport(VM, HostMod);
  // Inited phase
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Loaded phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Validated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Instantiated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // VM nullptr case
  Async = WasmEdge_VMAsyncExecute(nullptr, FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  Async = WasmEdge_VMAsyncExecute(VM, FuncName2, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_VMAsyncExecute(VM, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  // Async VM execute registered
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  WasmEdge_VMRegisterModuleFromImport(VM, HostMod);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBytes(
      VM, ModName,
      WasmEdge_BytesWrap(Buf.data(), static_cast<uint32_t>(Buf.size())))));
  // Success case
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // VM nullptr case
  Async = WasmEdge_VMAsyncExecuteRegistered(nullptr, ModName, FuncName, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Module not found
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName2, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function not found
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName2, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_VMAsyncExecuteRegistered(VM, ModName, FuncName, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  // Async Executor invoke
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_ExecutorRegisterImport(Exec, Store, HostMod)));
  WasmEdge_ModuleInstanceContext *ModInst = nullptr;
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInstantiate(Exec, &ModInst, Store, Mod)));
  EXPECT_NE(ModInst, nullptr);
  WasmEdge_FunctionInstanceContext *FuncInst =
      WasmEdge_ModuleInstanceFindFunction(ModInst, FuncName);
  EXPECT_NE(FuncInst, nullptr);
  // Success case
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 2);
  EXPECT_NE(Async, nullptr);
  WasmEdge_AsyncWait(Async);
  EXPECT_EQ(WasmEdge_AsyncGetReturnsLength(Async), 2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  WasmEdge_AsyncDelete(Async);
  // Executor nullptr case
  Async = WasmEdge_ExecutorAsyncInvoke(nullptr, FuncInst, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Function instance nullptr case
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, nullptr, P, 2);
  EXPECT_EQ(Async, nullptr);
  // Function type mismatch
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 1);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, nullptr, 0);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, nullptr, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_AsyncGet(Async, R, 2)));
  WasmEdge_AsyncDelete(Async);
  P[0] = WasmEdge_ValueGenI32(123);
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, R, 1)));
  WasmEdge_AsyncDelete(Async);
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(0, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  // Discard result
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 0)));
  WasmEdge_AsyncDelete(Async);
  // Discard result
  Async = WasmEdge_ExecutorAsyncInvoke(Exec, FuncInst, P, 2);
  EXPECT_NE(Async, nullptr);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_AsyncGet(Async, nullptr, 1)));
  WasmEdge_AsyncDelete(Async);

  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(FuncName2);
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);
  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ModuleInstanceDelete(HostMod);
  WasmEdge_VMDelete(VM);
  WasmEdge_StoreDelete(Store);
  WasmEdge_ExecutorDelete(Exec);
}

TEST(APICoreTest, VM) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule("extern");
  WasmEdge_String ModName, ModName2, FuncName, FuncName2, Names[20];
  WasmEdge_Value P[10], R[10];
  const WasmEdge_FunctionTypeContext *FuncTypes[15];

  // Prepare TPath
  HexToFile(TestWasm, TPath);
  // WASM from file
  std::vector<uint8_t> Buf;
  EXPECT_TRUE(readToVector(TPath, Buf));

  // Load and validate to wasm AST
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf, TPath);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  // VM creation and deletion
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(nullptr, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  EXPECT_TRUE(true);
  WasmEdge_VMDelete(nullptr);
  EXPECT_TRUE(true);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  VM = WasmEdge_VMCreate(nullptr, Store);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  VM = WasmEdge_VMCreate(Conf, Store);
  WasmEdge_ConfigureDelete(Conf);

  // VM register module from import
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_VMRegisterModuleFromImport(nullptr, HostMod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMRegisterModuleFromImport(VM, nullptr)));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_ModuleNameConflict,
                         WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));

  // VM register module from file
  ModName = WasmEdge_StringCreateByCString("reg-wasm-file");
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_VMRegisterModuleFromFile(nullptr, ModName, TPath)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_IllegalPath,
                 WasmEdge_VMRegisterModuleFromFile(VM, ModName, "no_file")));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromFile(VM, ModName, TPath)));
  WasmEdge_StringDelete(ModName);

  // VM register module from buffer
  ModName = WasmEdge_StringCreateByCString("reg-wasm-buffer");
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMRegisterModuleFromBuffer(nullptr, ModName, Buf.data(),
                                          static_cast<uint32_t>(Buf.size()))));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_UnexpectedEnd,
                 WasmEdge_VMRegisterModuleFromBuffer(VM, ModName, nullptr, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBuffer(
      VM, ModName, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  WasmEdge_StringDelete(ModName);

  // VM register module from AST module
  ModName = WasmEdge_StringCreateByCString("reg-wasm-ast");
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMRegisterModuleFromASTModule(nullptr, ModName, Mod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_ModuleNameConflict,
                 WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  WasmEdge_StringDelete(ModName);

  ModName = WasmEdge_StringCreateByCString("reg-wasm-buffer");
  ModName2 = WasmEdge_StringCreateByCString("reg-wasm-error");
  FuncName = WasmEdge_StringCreateByCString("func-mul-2");
  FuncName2 = WasmEdge_StringCreateByCString("func-mul-3");
  P[0] = WasmEdge_ValueGenI32(123);
  P[1] = WasmEdge_ValueGenI32(456);

  // VM run wasm from file
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMRunWasmFromFile(nullptr, TPath, FuncName, P, 2, R, 2)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_IllegalPath,
      WasmEdge_VMRunWasmFromFile(VM, "no_file", FuncName, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                 WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                 WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName2, P, 2, R, 1)));
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 0)));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 1)));

  // VM run wasm from buffer
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_VMRunWasmFromBuffer(nullptr, Buf.data(),
                                              static_cast<uint32_t>(Buf.size()),
                                              FuncName, P, 2, R, 2)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_UnexpectedEnd,
      WasmEdge_VMRunWasmFromBuffer(VM, nullptr, 0, FuncName, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMRunWasmFromBuffer(
                             VM, Buf.data(), static_cast<uint32_t>(Buf.size()),
                             FuncName, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMRunWasmFromBuffer(
                             VM, Buf.data(), static_cast<uint32_t>(Buf.size()),
                             FuncName, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMRunWasmFromBuffer(
                             VM, Buf.data(), static_cast<uint32_t>(Buf.size()),
                             FuncName, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMRunWasmFromBuffer(
                             VM, Buf.data(), static_cast<uint32_t>(Buf.size()),
                             FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_VMRunWasmFromBuffer(
                             VM, Buf.data(), static_cast<uint32_t>(Buf.size()),
                             FuncName2, P, 2, R, 2)));
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2,
      nullptr, 0)));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2,
      nullptr, 1)));

  // VM run wasm from AST module
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMRunWasmFromASTModule(nullptr, Mod, FuncName, P, 2, R, 2)));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMRunWasmFromASTModule(VM, nullptr, FuncName, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncNotFound,
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName2, P, 2, R, 2)));
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 0)));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 1)));

  // VM get registered module
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VM), 17U);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_VMListRegisteredModule(nullptr, Names, 20), 0U);
  EXPECT_EQ(WasmEdge_VMListRegisteredModule(VM, nullptr, 20), 17U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 20);
  EXPECT_EQ(WasmEdge_VMListRegisteredModule(VM, Names, 1), 17U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "extern"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), ""sv);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 20);
  EXPECT_EQ(WasmEdge_VMListRegisteredModule(VM, Names, 20), 17U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "extern"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "reg-wasm-ast"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length),
            "reg-wasm-buffer"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "reg-wasm-file"sv);

  // VM load wasm from file
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VM, TPath)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMLoadWasmFromFile(nullptr, TPath)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_IllegalPath,
                         WasmEdge_VMLoadWasmFromFile(VM, "file")));

  // VM load wasm from buffer
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                 WasmEdge_VMLoadWasmFromBuffer(
                     nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_UnexpectedEnd,
                         WasmEdge_VMLoadWasmFromBuffer(VM, nullptr, 0)));

  // VM load wasm from AST module
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMLoadWasmFromASTModule(nullptr, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMLoadWasmFromASTModule(VM, nullptr)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMLoadWasmFromASTModule(nullptr, nullptr)));

  // VM validate
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow, WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMValidate(nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));

  // VM instantiate
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow, WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(
      isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow, WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMInstantiate(nullptr)));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));

  // VM execute
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));
  // Inited phase
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  // Loaded phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  // Validated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongInstanceAddress,
                         WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  // Instantiated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_WrongVMWorkflow,
                         WasmEdge_VMExecute(nullptr, FuncName, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMExecute(VM, FuncName, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMExecute(VM, FuncName, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMExecute(VM, FuncName, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncSigMismatch,
                         WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Function not found
  EXPECT_TRUE(isErrMatch(WasmEdge_ErrCode_FuncNotFound,
                         WasmEdge_VMExecute(VM, FuncName2, P, 2, R, 2)));
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, nullptr, 0)));
  // Discard result
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, nullptr, 1)));

  // VM execute registered
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBytes(
      VM, ModName,
      WasmEdge_BytesWrap(Buf.data(), static_cast<uint32_t>(Buf.size())))));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[1].Type));
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongVMWorkflow,
      WasmEdge_VMExecuteRegistered(nullptr, ModName, FuncName, P, 2, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 1, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 0, R, 2)));
  // Function type mismatch
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 2, R, 2)));
  // Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncSigMismatch,
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  // Module not found
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_WrongInstanceAddress,
      WasmEdge_VMExecuteRegistered(VM, ModName2, FuncName, P, 2, R, 2)));
  // Function not found
  EXPECT_TRUE(isErrMatch(
      WasmEdge_ErrCode_FuncNotFound,
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName2, P, 2, R, 2)));
  // Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 0)));
  // Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 1)));

  // VM get function type
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, HostMod)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  EXPECT_NE(WasmEdge_VMGetFunctionType(VM, FuncName), nullptr);
  EXPECT_EQ(WasmEdge_VMGetFunctionType(nullptr, FuncName), nullptr);
  EXPECT_EQ(WasmEdge_VMGetFunctionType(VM, FuncName2), nullptr);

  // VM get function type registered
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBytes(
      VM, ModName,
      WasmEdge_BytesWrap(Buf.data(), static_cast<uint32_t>(Buf.size())))));
  EXPECT_NE(WasmEdge_VMGetFunctionTypeRegistered(VM, ModName, FuncName),
            nullptr);
  EXPECT_EQ(WasmEdge_VMGetFunctionTypeRegistered(nullptr, ModName, FuncName),
            nullptr);
  EXPECT_EQ(WasmEdge_VMGetFunctionTypeRegistered(VM, ModName2, FuncName),
            nullptr);
  EXPECT_EQ(WasmEdge_VMGetFunctionTypeRegistered(VM, ModName, FuncName2),
            nullptr);

  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(FuncName2);
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);

  // VM get function list
  EXPECT_EQ(WasmEdge_VMGetFunctionListLength(VM), 11U);
  EXPECT_EQ(WasmEdge_VMGetFunctionListLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(nullptr, Names, FuncTypes, 15), 0U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, nullptr, FuncTypes, 15), 11U);

  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, nullptr, 15), 11U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "func-1"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "func-2"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length), "func-3"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "func-4"sv);
  EXPECT_EQ(std::string_view(Names[4].Buf, Names[4].Length), "func-add"sv);
  EXPECT_EQ(std::string_view(Names[5].Buf, Names[5].Length),
            "func-call-indirect"sv);
  EXPECT_EQ(std::string_view(Names[6].Buf, Names[6].Length), "func-host-add"sv);
  EXPECT_EQ(std::string_view(Names[7].Buf, Names[7].Length), "func-host-div"sv);
  EXPECT_EQ(std::string_view(Names[8].Buf, Names[8].Length), "func-host-mul"sv);
  EXPECT_EQ(std::string_view(Names[9].Buf, Names[9].Length), "func-host-sub"sv);
  EXPECT_EQ(std::string_view(Names[10].Buf, Names[10].Length), "func-mul-2"sv);

  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, nullptr, nullptr, 15), 11U);

  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, FuncTypes, 4), 11U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "func-1"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "func-2"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length), "func-3"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "func-4"sv);

  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, FuncTypes, 15), 11U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), "func-1"sv);
  EXPECT_EQ(std::string_view(Names[1].Buf, Names[1].Length), "func-2"sv);
  EXPECT_EQ(std::string_view(Names[2].Buf, Names[2].Length), "func-3"sv);
  EXPECT_EQ(std::string_view(Names[3].Buf, Names[3].Length), "func-4"sv);
  EXPECT_EQ(std::string_view(Names[4].Buf, Names[4].Length), "func-add"sv);
  EXPECT_EQ(std::string_view(Names[5].Buf, Names[5].Length),
            "func-call-indirect"sv);
  EXPECT_EQ(std::string_view(Names[6].Buf, Names[6].Length), "func-host-add"sv);
  EXPECT_EQ(std::string_view(Names[7].Buf, Names[7].Length), "func-host-div"sv);
  EXPECT_EQ(std::string_view(Names[8].Buf, Names[8].Length), "func-host-mul"sv);
  EXPECT_EQ(std::string_view(Names[9].Buf, Names[9].Length), "func-host-sub"sv);
  EXPECT_EQ(std::string_view(Names[10].Buf, Names[10].Length), "func-mul-2"sv);

  // VM get active module
  EXPECT_NE(WasmEdge_VMGetActiveModule(VM), nullptr);
  EXPECT_EQ(
      WasmEdge_ModuleInstanceListFunctionLength(WasmEdge_VMGetActiveModule(VM)),
      11U);
  EXPECT_EQ(WasmEdge_VMGetActiveModule(nullptr), nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunctionLength(
                WasmEdge_VMGetActiveModule(nullptr)),
            0U);
  WasmEdge_VMCleanup(VM);
  EXPECT_EQ(WasmEdge_VMGetActiveModule(VM), nullptr);

  // VM cleanup
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(true);
  WasmEdge_VMCleanup(nullptr);
  EXPECT_TRUE(true);

  // VM get pre-registered module (WASI)
  EXPECT_NE(
      WasmEdge_VMGetImportModuleContext(VM, WasmEdge_HostRegistration_Wasi),
      nullptr);
  EXPECT_EQ(WasmEdge_VMGetImportModuleContext(nullptr,
                                              WasmEdge_HostRegistration_Wasi),
            nullptr);

  // VM get registered module (plug-ins)
  ModName = WasmEdge_StringCreateByCString("wasi_ephemeral_nn");
  EXPECT_NE(WasmEdge_VMGetRegisteredModule(VM, ModName), nullptr);
  EXPECT_EQ(WasmEdge_VMGetRegisteredModule(nullptr, ModName), nullptr);
  WasmEdge_StringDelete(ModName);
  ModName = WasmEdge_StringCreateByCString("no-such-plugin");
  EXPECT_EQ(WasmEdge_VMGetRegisteredModule(VM, ModName), nullptr);
  WasmEdge_StringDelete(ModName);

  // VM get store
  EXPECT_EQ(WasmEdge_VMGetStoreContext(VM), Store);
  EXPECT_EQ(WasmEdge_VMGetStoreContext(nullptr), nullptr);

  // VM get loader
  EXPECT_NE(WasmEdge_VMGetLoaderContext(VM), nullptr);
  EXPECT_EQ(WasmEdge_VMGetLoaderContext(nullptr), nullptr);

  // VM get validator
  EXPECT_NE(WasmEdge_VMGetValidatorContext(VM), nullptr);
  EXPECT_EQ(WasmEdge_VMGetValidatorContext(nullptr), nullptr);

  // VM get executor
  EXPECT_NE(WasmEdge_VMGetExecutorContext(VM), nullptr);
  EXPECT_EQ(WasmEdge_VMGetExecutorContext(nullptr), nullptr);

  // VM get statistics
  EXPECT_NE(WasmEdge_VMGetStatisticsContext(VM), nullptr);
  EXPECT_EQ(WasmEdge_VMGetStatisticsContext(nullptr), nullptr);

  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ModuleInstanceDelete(HostMod);
  WasmEdge_StoreDelete(Store);
  WasmEdge_VMDelete(VM);
}

#if defined(WASMEDGE_BUILD_PLUGINS)
TEST(APICoreTest, Plugin) {
  WasmEdge_String Names[15];

  // Load from the specific path
  EXPECT_EQ(WasmEdge_PluginListPluginsLength(), 0U);
  WasmEdge_PluginLoadFromPath(
      "../plugins/unittest/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginTestModuleCPP" WASMEDGE_LIB_EXTENSION);
  EXPECT_EQ(WasmEdge_PluginListPluginsLength(), 1U);

  // Get the loaded plugin length
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_PluginListPlugins(nullptr, 0), 1U);
  EXPECT_EQ(WasmEdge_PluginListPlugins(Names, 0), 1U);
  EXPECT_EQ(WasmEdge_PluginListPlugins(Names, 15), 1U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length),
            "wasmedge_plugintest_cpp"sv);

  // Find the plugin context
  const WasmEdge_PluginContext *PluginCxt =
      WasmEdge_PluginFind(WasmEdge_StringWrap("no-such-plugin-name", 19));
  EXPECT_EQ(PluginCxt, nullptr);
  PluginCxt = WasmEdge_PluginFind(Names[0]);
  EXPECT_NE(PluginCxt, nullptr);

  // Get plugin name
  Names[0] = WasmEdge_PluginGetPluginName(PluginCxt);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length),
            "wasmedge_plugintest_cpp"sv);
  Names[0] = WasmEdge_PluginGetPluginName(nullptr);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), ""sv);

  // List modules in the plugin
  EXPECT_EQ(WasmEdge_PluginListModuleLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_PluginListModuleLength(PluginCxt), 1U);
  std::memset(Names, 0, sizeof(WasmEdge_String) * 15);
  EXPECT_EQ(WasmEdge_PluginListModule(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_PluginListModule(nullptr, nullptr, 0), 0U);
  EXPECT_EQ(WasmEdge_PluginListModule(PluginCxt, nullptr, 0), 1U);
  EXPECT_EQ(WasmEdge_PluginListModule(PluginCxt, Names, 0), 1U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length), ""sv);
  EXPECT_EQ(WasmEdge_PluginListModule(PluginCxt, Names, 15), 1U);
  EXPECT_EQ(std::string_view(Names[0].Buf, Names[0].Length),
            "wasmedge_plugintest_cpp_module"sv);

  // Create the module
  WasmEdge_ModuleInstanceContext *ModCxt =
      WasmEdge_PluginCreateModule(nullptr, Names[0]);
  EXPECT_EQ(ModCxt, nullptr);
  ModCxt = WasmEdge_PluginCreateModule(
      PluginCxt, WasmEdge_StringWrap("no-such-plugin-name", 19));
  EXPECT_EQ(ModCxt, nullptr);
  ModCxt = WasmEdge_PluginCreateModule(PluginCxt, Names[0]);
  EXPECT_NE(ModCxt, nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModCxt, Names, 15), 5U);
  WasmEdge_ModuleInstanceDelete(ModCxt);
}
#endif
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
