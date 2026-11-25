// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/thread/ThreadTest.cpp - Multithread test ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for running Executor in parallel.
///
//===----------------------------------------------------------------------===//

#include "common/filesystem.h"
#include "common/spdlog.h"
#include "vm/vm.h"

#ifdef WASMEDGE_USE_LLVM
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#endif

#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

// See mt19937.c for source of this webassembly data.
std::array<WasmEdge::Byte, 1481> MersenneTwister19937{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x01, 0x60,
    0x03, 0x7f, 0x7e, 0x7e, 0x01, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03,
    0x01, 0x00, 0x01, 0x07, 0x0b, 0x01, 0x07, 0x6d, 0x74, 0x31, 0x39, 0x39,
    0x33, 0x37, 0x00, 0x00, 0x0a, 0x9e, 0x0b, 0x01, 0x9b, 0x0b, 0x03, 0x02,
    0x7f, 0x03, 0x7e, 0x01, 0x7b, 0x20, 0x00, 0x20, 0x01, 0x37, 0x03, 0x00,
    0x20, 0x00, 0x41, 0x20, 0x6a, 0x21, 0x03, 0x42, 0x01, 0x21, 0x06, 0x42,
    0x04, 0x21, 0x05, 0x03, 0x7e, 0x20, 0x03, 0x41, 0x18, 0x6b, 0x20, 0x05,
    0x20, 0x01, 0x42, 0x3e, 0x88, 0x20, 0x01, 0x85, 0x42, 0xad, 0xfe, 0xd5,
    0xe4, 0xd4, 0x85, 0xfd, 0xa8, 0xd8, 0x00, 0x7e, 0x7c, 0x42, 0x03, 0x7d,
    0x22, 0x01, 0x37, 0x03, 0x00, 0x20, 0x03, 0x41, 0x10, 0x6b, 0x20, 0x05,
    0x20, 0x01, 0x42, 0x3e, 0x88, 0x20, 0x01, 0x85, 0x42, 0xad, 0xfe, 0xd5,
    0xe4, 0xd4, 0x85, 0xfd, 0xa8, 0xd8, 0x00, 0x7e, 0x7c, 0x42, 0x02, 0x7d,
    0x22, 0x01, 0x37, 0x03, 0x00, 0x20, 0x03, 0x41, 0x08, 0x6b, 0x20, 0x05,
    0x20, 0x01, 0x42, 0x3e, 0x88, 0x20, 0x01, 0x85, 0x42, 0xad, 0xfe, 0xd5,
    0xe4, 0xd4, 0x85, 0xfd, 0xa8, 0xd8, 0x00, 0x7e, 0x7c, 0x42, 0x01, 0x7d,
    0x22, 0x01, 0x37, 0x03, 0x00, 0x20, 0x05, 0x42, 0xb8, 0x02, 0x51, 0x04,
    0x7e, 0x20, 0x00, 0x42, 0xb8, 0x02, 0x37, 0x03, 0xc0, 0x13, 0x02, 0x40,
    0x20, 0x02, 0x50, 0x04, 0x40, 0x42, 0x00, 0x21, 0x02, 0x42, 0xb8, 0x02,
    0x21, 0x05, 0x0c, 0x01, 0x0b, 0x42, 0x00, 0x21, 0x01, 0x03, 0x40, 0x42,
    0x9c, 0x01, 0x21, 0x05, 0x20, 0x00, 0x21, 0x03, 0x03, 0x40, 0x20, 0x03,
    0x20, 0x03, 0xfd, 0x00, 0x03, 0x08, 0x22, 0x08, 0xfd, 0x0c, 0xfe, 0xff,
    0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00,
    0x00, 0x00, 0xfd, 0x4e, 0x20, 0x03, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x0c,
    0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x80,
    0xff, 0xff, 0xff, 0xff, 0xfd, 0x4e, 0xfd, 0x50, 0x41, 0x01, 0xfd, 0xcd,
    0x01, 0x20, 0x03, 0x41, 0xe0, 0x09, 0x6a, 0xfd, 0x00, 0x03, 0x00, 0xfd,
    0x51, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x0c, 0xe9, 0x19, 0x66,
    0xa9, 0x5a, 0x6f, 0x02, 0xb5, 0xe9, 0x19, 0x66, 0xa9, 0x5a, 0x6f, 0x02,
    0xb5, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0xfd, 0x0c, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfd, 0x4e, 0xfd, 0xd6, 0x01, 0xfd, 0x52, 0xfd, 0x51,
    0xfd, 0x0b, 0x03, 0x00, 0x20, 0x03, 0x41, 0x10, 0x6a, 0x21, 0x03, 0x20,
    0x05, 0x42, 0x02, 0x7d, 0x22, 0x05, 0x42, 0x00, 0x52, 0x0d, 0x00, 0x0b,
    0x42, 0x9a, 0x01, 0x21, 0x05, 0x20, 0x00, 0x21, 0x03, 0x03, 0x40, 0x20,
    0x03, 0x41, 0xe0, 0x09, 0x6a, 0x22, 0x04, 0x20, 0x03, 0x41, 0xe8, 0x09,
    0x6a, 0xfd, 0x00, 0x03, 0x00, 0x22, 0x08, 0xfd, 0x0c, 0xfe, 0xff, 0xff,
    0x7f, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00,
    0x00, 0xfd, 0x4e, 0x20, 0x04, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x0c, 0x00,
    0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x80, 0xff,
    0xff, 0xff, 0xff, 0xfd, 0x4e, 0xfd, 0x50, 0x41, 0x01, 0xfd, 0xcd, 0x01,
    0x20, 0x03, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x51, 0xfd, 0x0c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xfd, 0x0c, 0xe9, 0x19, 0x66, 0xa9, 0x5a, 0x6f, 0x02, 0xb5,
    0xe9, 0x19, 0x66, 0xa9, 0x5a, 0x6f, 0x02, 0xb5, 0x20, 0x08, 0xfd, 0x0c,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xfd, 0x4e, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xfd, 0xd6, 0x01, 0xfd, 0x52, 0xfd, 0x51, 0xfd, 0x0b, 0x03, 0x00, 0x20,
    0x03, 0x41, 0x10, 0x6a, 0x21, 0x03, 0x20, 0x05, 0x42, 0x02, 0x7d, 0x22,
    0x05, 0x42, 0x00, 0x52, 0x0d, 0x00, 0x0b, 0x42, 0x00, 0x21, 0x05, 0x20,
    0x00, 0x42, 0x00, 0x37, 0x03, 0xc0, 0x13, 0x20, 0x00, 0x42, 0x00, 0x20,
    0x00, 0x29, 0x03, 0xb8, 0x13, 0x22, 0x06, 0x42, 0x01, 0x83, 0x7d, 0x42,
    0xe9, 0xb3, 0x98, 0xcb, 0xaa, 0xeb, 0x9b, 0x81, 0xb5, 0x7f, 0x83, 0x20,
    0x00, 0x29, 0x03, 0xd0, 0x09, 0x20, 0x06, 0x42, 0xfe, 0xff, 0xff, 0xff,
    0x07, 0x83, 0x20, 0x00, 0x29, 0x03, 0xb0, 0x13, 0x42, 0x80, 0x80, 0x80,
    0x80, 0x78, 0x83, 0x84, 0x42, 0x01, 0x88, 0x85, 0x85, 0x37, 0x03, 0xb0,
    0x13, 0x20, 0x00, 0x42, 0x00, 0x20, 0x00, 0x29, 0x03, 0x00, 0x22, 0x07,
    0x42, 0x01, 0x83, 0x7d, 0x42, 0xe9, 0xb3, 0x98, 0xcb, 0xaa, 0xeb, 0x9b,
    0x81, 0xb5, 0x7f, 0x83, 0x20, 0x00, 0x29, 0x03, 0xd8, 0x09, 0x20, 0x07,
    0x42, 0xfe, 0xff, 0xff, 0xff, 0x07, 0x83, 0x20, 0x06, 0x42, 0x80, 0x80,
    0x80, 0x80, 0x78, 0x83, 0x84, 0x42, 0x01, 0x88, 0x85, 0x85, 0x37, 0x03,
    0xb8, 0x13, 0x20, 0x01, 0x20, 0x02, 0x7c, 0x21, 0x02, 0x42, 0xc8, 0x7d,
    0x21, 0x01, 0x20, 0x02, 0x42, 0xb8, 0x02, 0x56, 0x0d, 0x00, 0x0b, 0x0b,
    0x20, 0x00, 0x20, 0x02, 0x20, 0x05, 0x7c, 0x22, 0x05, 0x37, 0x03, 0xc0,
    0x13, 0x20, 0x05, 0x42, 0xb8, 0x02, 0x5a, 0x04, 0x40, 0x42, 0x9c, 0x01,
    0x21, 0x05, 0x20, 0x00, 0x21, 0x03, 0x03, 0x40, 0x20, 0x03, 0x20, 0x03,
    0xfd, 0x00, 0x03, 0x08, 0x22, 0x08, 0xfd, 0x0c, 0xfe, 0xff, 0xff, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00,
    0xfd, 0x4e, 0x20, 0x03, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x0c, 0x00, 0x00,
    0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff,
    0xff, 0xff, 0xfd, 0x4e, 0xfd, 0x50, 0x41, 0x01, 0xfd, 0xcd, 0x01, 0x20,
    0x03, 0x41, 0xe0, 0x09, 0x6a, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x51, 0xfd,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x0c, 0xe9, 0x19, 0x66, 0xa9, 0x5a,
    0x6f, 0x02, 0xb5, 0xe9, 0x19, 0x66, 0xa9, 0x5a, 0x6f, 0x02, 0xb5, 0xfd,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0xfd, 0x0c, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xfd, 0x4e, 0xfd, 0xd6, 0x01, 0xfd, 0x52, 0xfd, 0x51, 0xfd, 0x0b,
    0x03, 0x00, 0x20, 0x03, 0x41, 0x10, 0x6a, 0x21, 0x03, 0x20, 0x05, 0x42,
    0x02, 0x7d, 0x22, 0x05, 0x42, 0x00, 0x52, 0x0d, 0x00, 0x0b, 0x42, 0x9a,
    0x01, 0x21, 0x05, 0x20, 0x00, 0x21, 0x03, 0x03, 0x40, 0x20, 0x03, 0x41,
    0xe0, 0x09, 0x6a, 0x22, 0x04, 0x20, 0x03, 0x41, 0xe8, 0x09, 0x6a, 0xfd,
    0x00, 0x03, 0x00, 0x22, 0x08, 0xfd, 0x0c, 0xfe, 0xff, 0xff, 0x7f, 0x00,
    0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xfd,
    0x4e, 0x20, 0x04, 0xfd, 0x00, 0x03, 0x00, 0xfd, 0x0c, 0x00, 0x00, 0x00,
    0x80, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff,
    0xff, 0xfd, 0x4e, 0xfd, 0x50, 0x41, 0x01, 0xfd, 0xcd, 0x01, 0x20, 0x03,
    0xfd, 0x00, 0x03, 0x00, 0xfd, 0x51, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xfd, 0x0c, 0xe9, 0x19, 0x66, 0xa9, 0x5a, 0x6f, 0x02, 0xb5, 0xe9, 0x19,
    0x66, 0xa9, 0x5a, 0x6f, 0x02, 0xb5, 0x20, 0x08, 0xfd, 0x0c, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xfd, 0x4e, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xd6,
    0x01, 0xfd, 0x52, 0xfd, 0x51, 0xfd, 0x0b, 0x03, 0x00, 0x20, 0x03, 0x41,
    0x10, 0x6a, 0x21, 0x03, 0x20, 0x05, 0x42, 0x02, 0x7d, 0x22, 0x05, 0x42,
    0x00, 0x52, 0x0d, 0x00, 0x0b, 0x20, 0x00, 0x42, 0x00, 0x20, 0x00, 0x29,
    0x03, 0xb8, 0x13, 0x22, 0x01, 0x42, 0x01, 0x83, 0x7d, 0x42, 0xe9, 0xb3,
    0x98, 0xcb, 0xaa, 0xeb, 0x9b, 0x81, 0xb5, 0x7f, 0x83, 0x20, 0x00, 0x29,
    0x03, 0xd0, 0x09, 0x20, 0x01, 0x42, 0xfe, 0xff, 0xff, 0xff, 0x07, 0x83,
    0x20, 0x00, 0x29, 0x03, 0xb0, 0x13, 0x42, 0x80, 0x80, 0x80, 0x80, 0x78,
    0x83, 0x84, 0x42, 0x01, 0x88, 0x85, 0x85, 0x37, 0x03, 0xb0, 0x13, 0x20,
    0x00, 0x42, 0x00, 0x20, 0x00, 0x29, 0x03, 0x00, 0x22, 0x02, 0x42, 0x01,
    0x83, 0x7d, 0x42, 0xe9, 0xb3, 0x98, 0xcb, 0xaa, 0xeb, 0x9b, 0x81, 0xb5,
    0x7f, 0x83, 0x20, 0x00, 0x29, 0x03, 0xd8, 0x09, 0x20, 0x02, 0x42, 0xfe,
    0xff, 0xff, 0xff, 0x07, 0x83, 0x20, 0x01, 0x42, 0x80, 0x80, 0x80, 0x80,
    0x78, 0x83, 0x84, 0x42, 0x01, 0x88, 0x85, 0x85, 0x37, 0x03, 0xb8, 0x13,
    0x42, 0x00, 0x21, 0x05, 0x0b, 0x20, 0x00, 0x20, 0x05, 0x42, 0x01, 0x7c,
    0x37, 0x03, 0xc0, 0x13, 0x20, 0x00, 0x20, 0x05, 0xa7, 0x41, 0x03, 0x74,
    0x6a, 0x29, 0x03, 0x00, 0x22, 0x01, 0x42, 0x1d, 0x88, 0x42, 0xd5, 0xaa,
    0xd5, 0xaa, 0xd5, 0x00, 0x83, 0x20, 0x01, 0x85, 0x22, 0x01, 0x42, 0x11,
    0x86, 0x42, 0x80, 0x80, 0x98, 0xed, 0xfe, 0xff, 0x9f, 0xeb, 0xf1, 0x00,
    0x83, 0x20, 0x01, 0x85, 0x22, 0x01, 0x42, 0x25, 0x86, 0x42, 0x80, 0x80,
    0x80, 0x80, 0x80, 0xdc, 0xfb, 0x7b, 0x83, 0x20, 0x01, 0x85, 0x22, 0x01,
    0x42, 0x2b, 0x88, 0x20, 0x01, 0x85, 0x05, 0x20, 0x06, 0x42, 0x03, 0x7c,
    0x21, 0x07, 0x20, 0x03, 0x20, 0x01, 0x42, 0x3e, 0x88, 0x20, 0x01, 0x85,
    0x42, 0xad, 0xfe, 0xd5, 0xe4, 0xd4, 0x85, 0xfd, 0xa8, 0xd8, 0x00, 0x7e,
    0x22, 0x01, 0x20, 0x05, 0x7c, 0x37, 0x03, 0x00, 0x20, 0x03, 0x41, 0x20,
    0x6a, 0x21, 0x03, 0x20, 0x05, 0x42, 0x04, 0x7c, 0x21, 0x05, 0x20, 0x06,
    0x42, 0x04, 0x7c, 0x21, 0x06, 0x20, 0x01, 0x20, 0x07, 0x7c, 0x21, 0x01,
    0x0c, 0x01, 0x0b, 0x0b, 0x0b,
};
std::array<uint64_t, 4> Answers{
    UINT64_C(7605900683918645917),
    UINT64_C(9082641531226583590),
    UINT64_C(4446454406775736720),
    UINT64_C(9019442596657776185),
};
std::array<WasmEdge::Byte, 122> AtomicThreads32{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x15, 0x03, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x03, 0x7f, 0x7f, 0x7e, 0x01, 0x7f,
    0x60, 0x03, 0x7f, 0x7e, 0x7e, 0x01, 0x7f, 0x03, 0x04, 0x03, 0x00, 0x01,
    0x02, 0x05, 0x04, 0x01, 0x03, 0x01, 0x01, 0x07, 0x25, 0x04, 0x06, 0x6d,
    0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x06, 0x6e, 0x6f, 0x74, 0x69,
    0x66, 0x79, 0x00, 0x00, 0x06, 0x77, 0x61, 0x69, 0x74, 0x33, 0x32, 0x00,
    0x01, 0x06, 0x77, 0x61, 0x69, 0x74, 0x36, 0x34, 0x00, 0x02, 0x0a, 0x26,
    0x03, 0x0a, 0x00, 0x20, 0x00, 0x20, 0x01, 0xfe, 0x00, 0x02, 0x00, 0x0b,
    0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x01, 0x02, 0x00,
    0x0b, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x02, 0x03,
    0x00, 0x0b,
};
std::array<WasmEdge::Byte, 255> AtomicThreads64{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1a, 0x04, 0x60,
    0x02, 0x7e, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7e, 0x01, 0x7f, 0x60, 0x03,
    0x7e, 0x7f, 0x7e, 0x01, 0x7f, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x01, 0x7f,
    0x03, 0x08, 0x07, 0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x05, 0x04,
    0x01, 0x07, 0x01, 0x01, 0x07, 0x6c, 0x08, 0x06, 0x6d, 0x65, 0x6d, 0x6f,
    0x72, 0x79, 0x02, 0x00, 0x06, 0x6e, 0x6f, 0x74, 0x69, 0x66, 0x79, 0x00,
    0x00, 0x0d, 0x6e, 0x6f, 0x74, 0x69, 0x66, 0x79, 0x2d, 0x6f, 0x66, 0x66,
    0x73, 0x65, 0x74, 0x00, 0x01, 0x14, 0x6e, 0x6f, 0x74, 0x69, 0x66, 0x79,
    0x2d, 0x77, 0x72, 0x61, 0x70, 0x70, 0x65, 0x64, 0x2d, 0x63, 0x6f, 0x75,
    0x6e, 0x74, 0x00, 0x02, 0x06, 0x77, 0x61, 0x69, 0x74, 0x33, 0x32, 0x00,
    0x03, 0x0d, 0x77, 0x61, 0x69, 0x74, 0x33, 0x32, 0x2d, 0x6f, 0x66, 0x66,
    0x73, 0x65, 0x74, 0x00, 0x04, 0x06, 0x77, 0x61, 0x69, 0x74, 0x36, 0x34,
    0x00, 0x05, 0x0d, 0x77, 0x61, 0x69, 0x74, 0x36, 0x34, 0x2d, 0x6f, 0x66,
    0x66, 0x73, 0x65, 0x74, 0x00, 0x06, 0x0a, 0x5b, 0x07, 0x0a, 0x00, 0x20,
    0x00, 0x20, 0x01, 0xfe, 0x00, 0x02, 0x00, 0x0b, 0x0a, 0x00, 0x20, 0x00,
    0x20, 0x01, 0xfe, 0x00, 0x02, 0x04, 0x0b, 0x0f, 0x00, 0x20, 0x00, 0x42,
    0x81, 0x80, 0x80, 0x80, 0x20, 0xa7, 0xfe, 0x00, 0x02, 0x00, 0x0b, 0x0c,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x01, 0x02, 0x00, 0x0b,
    0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x01, 0x02, 0x04,
    0x0b, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x02, 0x03,
    0x00, 0x0b, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x02,
    0x03, 0x08, 0x0b,
};

using namespace std::literals;

TEST(AtomicWaitNotify, Memory32) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads32));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Result = VM.execute(
        "notify",
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(4), UINT32_C(1)},
        {WasmEdge::ValType(WasmEdge::TypeCode::I32),
         WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65533), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(2));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65533), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
}

TEST(AtomicWaitNotify, Memory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads64));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Result = VM.execute(
        "notify",
        std::initializer_list<WasmEdge::ValVariant>{UINT64_C(4), UINT32_C(1)},
        {WasmEdge::ValType(WasmEdge::TypeCode::I64),
         WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65533), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(0xFFFFFFFFFFFFFFFC), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(0), UINT32_C(0), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(2));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65533), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result =
        VM.execute("wait32-offset",
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT64_C(0xFFFFFFFFFFFFFFFC), UINT32_C(1), UINT64_C(0)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I32),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65520), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result =
        VM.execute("wait64-offset",
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT64_C(0xFFFFFFFFFFFFFFF8), UINT64_C(1), UINT64_C(0)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  auto *MemInst = VM.getActiveModule()->findMemoryExports("memory");
  ASSERT_NE(MemInst, nullptr);
  {
    std::unique_lock<std::mutex> Locker(MemInst->getWaiterMapMutex());
    auto &Queue = MemInst->getWaiterMap()[UINT64_C(8)];
    Queue.emplace_back();
    Queue.emplace_back();
  }
  {
    auto Result =
        VM.execute("notify-wrapped-count",
                   std::initializer_list<WasmEdge::ValVariant>{UINT64_C(8)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
}

/// A waiter leaves the waiter map when it returns, also after the map moved
/// it to a new bucket array.
TEST(AtomicWaitNotify, RehashWhileWaiting) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads32));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &MemInst = *VM.getActiveModule()->findMemoryExports("memory"sv);
  auto &Mutex = MemInst.getWaiterMapMutex();
  auto &WaiterMap = MemInst.getWaiterMap();
  constexpr auto Bound = 20s;
  const auto Timeout = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(Bound).count());

  auto Waiter = VM.asyncExecute("wait32"sv,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    UINT32_C(0), UINT32_C(0), Timeout},
                                {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                                 WasmEdge::ValType(WasmEdge::TypeCode::I32),
                                 WasmEdge::ValType(WasmEdge::TypeCode::I64)});
  struct Join {
    WasmEdge::VM::VM &VM;
    decltype(Waiter) &Worker;
    ~Join() {
      VM.stop();
      Worker.wait();
    }
  } Joined{VM, Waiter};
  bool Registered = false;
  const auto Deadline = std::chrono::steady_clock::now() + Bound;
  while (!Registered && std::chrono::steady_clock::now() < Deadline) {
    {
      std::unique_lock<std::mutex> Locker(Mutex);
      if (!WaiterMap.empty()) {
        WaiterMap.rehash(WaiterMap.bucket_count() * 64);
        Registered = true;
      }
    }
    std::this_thread::sleep_for(1ms);
  }
  EXPECT_TRUE(Registered);

  auto Notified = VM.execute(
      "notify"sv,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(1)},
      {WasmEdge::ValType(WasmEdge::TypeCode::I32),
       WasmEdge::ValType(WasmEdge::TypeCode::I32)});
  ASSERT_TRUE(Notified);
  EXPECT_EQ((*Notified)[0].first.get<uint32_t>(), UINT32_C(1));
  ASSERT_TRUE(Waiter.waitFor(Bound));
  auto Result = Waiter.get();
  ASSERT_TRUE(Result);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  std::unique_lock<std::mutex> Locker(Mutex);
  EXPECT_TRUE(WaiterMap.empty());
}

/// A notify counts only the waiters that it wakes. A waiter stays in the
/// queue until its thread returns, so a later notify skips a waiter that is
/// already woken, whose wait already timed out, or whose run was stopped.
TEST(AtomicWaitNotify, NotifyCountsOnlyWokenWaiters) {
  using WaitState =
      WasmEdge::Runtime::Instance::MemoryInstance::Waiter::WaitState;
  const auto Ended = {WaitState::TimedOut, WaitState::Interrupted,
                      WaitState::Notified};
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads32));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &MemInst = *VM.getActiveModule()->findMemoryExports("memory"sv);
  {
    std::unique_lock<std::mutex> Locker(MemInst.getWaiterMapMutex());
    auto &Queue = MemInst.getWaiterMap()[UINT64_C(0)];
    for (const auto State : Ended) {
      Queue.emplace_back().State = State;
    }
    Queue.emplace_back();
    Queue.emplace_back();
  }
  for (const uint32_t Woken : {1U, 1U, 0U}) {
    auto Notified = VM.execute(
        "notify"sv,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(1)},
        {WasmEdge::ValType(WasmEdge::TypeCode::I32),
         WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Notified);
    EXPECT_EQ((*Notified)[0].first.get<uint32_t>(), Woken);
  }
  std::unique_lock<std::mutex> Locker(MemInst.getWaiterMapMutex());
  auto Waiter = MemInst.getWaiterMap()[UINT64_C(0)].begin();
  for (const auto State : Ended) {
    EXPECT_EQ((Waiter++)->State, State);
  }
}

// (module
//   (memory (export "memory") 1 1 shared)
//   (func (export "wait32") (param i32 i32 i64) (result i32)
//     (memory.atomic.wait32 (local.get 0) (local.get 1) (local.get 2)))
//   (func (export "notify") (param i32 i32) (result i32)
//     (memory.atomic.notify (local.get 0) (local.get 1)))
//   (func (export "spin")
//     (i32.atomic.store (i32.const 16) (i32.const 1))
//     (loop (br_if 0 (i32.eqz (i32.atomic.load (i32.const 20))))))
//   (func (export "started") (result i32) (i32.atomic.load (i32.const 16)))
//   (func (export "release") (i32.atomic.store (i32.const 20) (i32.const 1))))
std::array<WasmEdge::Byte, 172> StopWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x15, 0x04, 0x60,
    0x03, 0x7f, 0x7f, 0x7e, 0x01, 0x7f, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f,
    0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x06, 0x05, 0x00, 0x01,
    0x02, 0x03, 0x02, 0x05, 0x04, 0x01, 0x03, 0x01, 0x01, 0x07, 0x37, 0x06,
    0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x06, 0x77, 0x61,
    0x69, 0x74, 0x33, 0x32, 0x00, 0x00, 0x06, 0x6e, 0x6f, 0x74, 0x69, 0x66,
    0x79, 0x00, 0x01, 0x04, 0x73, 0x70, 0x69, 0x6e, 0x00, 0x02, 0x07, 0x73,
    0x74, 0x61, 0x72, 0x74, 0x65, 0x64, 0x00, 0x03, 0x07, 0x72, 0x65, 0x6c,
    0x65, 0x61, 0x73, 0x65, 0x00, 0x04, 0x0a, 0x44, 0x05, 0x0c, 0x00, 0x20,
    0x00, 0x20, 0x01, 0x20, 0x02, 0xfe, 0x01, 0x02, 0x00, 0x0b, 0x0a, 0x00,
    0x20, 0x00, 0x20, 0x01, 0xfe, 0x00, 0x02, 0x00, 0x0b, 0x16, 0x00, 0x41,
    0x10, 0x41, 0x01, 0xfe, 0x17, 0x02, 0x00, 0x03, 0x40, 0x41, 0x14, 0xfe,
    0x10, 0x02, 0x00, 0x45, 0x0d, 0x00, 0x0b, 0x0b, 0x08, 0x00, 0x41, 0x10,
    0xfe, 0x10, 0x02, 0x00, 0x0b, 0x0a, 0x00, 0x41, 0x14, 0x41, 0x01, 0xfe,
    0x17, 0x02, 0x00, 0x0b,
};

// (module
//   (import "host" "gate" (func $gate))
//   (start $gate))
std::array<WasmEdge::Byte, 32> GateWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
    0x60, 0x00, 0x00, 0x02, 0x0d, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74,
    0x04, 0x67, 0x61, 0x74, 0x65, 0x00, 0x00, 0x08, 0x01, 0x00,
};

/// Host function that blocks until the test opens it. The VM holds its lock
/// while it instantiates a module, so a start function that calls the gate
/// keeps every other run from starting.
class Gate : public WasmEdge::Runtime::HostFunction<Gate> {
public:
  WasmEdge::Expect<void> body(const WasmEdge::Runtime::CallingFrame &) {
    Entered.set_value();
    Opened.wait();
    return {};
  }
  std::promise<void> Entered;
  std::shared_future<void> Opened;
};

class StopTest : public ::testing::Test {
protected:
  using AsyncT = decltype(std::declval<WasmEdge::VM::VM &>().asyncExecute(
      std::string_view{}));

  static constexpr const std::string_view Other = "other"sv;
  static constexpr const auto Bound = 20s;

  void SetUp() override {
    Conf.addProposal(WasmEdge::Proposal::Threads);
    VM = std::make_unique<WasmEdge::VM::VM>(Conf);
    ASSERT_TRUE(VM->registerModule(Other, StopWasm));
    ASSERT_TRUE(VM->loadWasm(StopWasm));
    ASSERT_TRUE(VM->validate());
    ASSERT_TRUE(VM->instantiate());
  }

  void TearDown() override {
    if (Locker.valid()) {
      openGate();
    }
    if (VM) {
      VM->execute("release"sv);
      VM->stop();
      wakeWaiters({});
      wakeWaiters(Other);
    }
    for (auto &Worker : Workers) {
      Worker.wait();
    }
  }

  WasmEdge::Runtime::Instance::MemoryInstance &
  memory(std::string_view ModName = {}) {
    const auto *ModInst = ModName.empty()
                              ? VM->getActiveModule()
                              : VM->getStoreManager().findModule(ModName);
    return *ModInst->findMemoryExports("memory"sv);
  }

  /// Wake every waiter of the memory without a stop request, so that
  /// TearDown() joins the workers also when a stop does not wake them.
  void wakeWaiters(std::string_view ModName) {
    const auto *ModInst = ModName.empty()
                              ? VM->getActiveModule()
                              : VM->getStoreManager().findModule(ModName);
    if (!ModInst) {
      return;
    }
    auto *MemInst = ModInst->findMemoryExports("memory"sv);
    using WaitState =
        WasmEdge::Runtime::Instance::MemoryInstance::Waiter::WaitState;
    std::unique_lock<std::mutex> Locker(MemInst->getWaiterMapMutex());
    for (auto &[Address, Queue] : MemInst->getWaiterMap()) {
      for (auto &Waiter : Queue) {
        Waiter.wake(WaitState::Notified);
      }
    }
  }

  bool waitForSpinner() {
    const auto Deadline = std::chrono::steady_clock::now() + Bound;
    while (std::chrono::steady_clock::now() < Deadline) {
      if (auto Started = toU32(VM->execute("started"sv)); Started && *Started) {
        return true;
      }
      std::this_thread::sleep_for(1ms);
    }
    return false;
  }

  bool waitForWaiters(std::string_view ModName, size_t Count) {
    auto &MemInst = memory(ModName);
    const auto Deadline = std::chrono::steady_clock::now() + Bound;
    while (std::chrono::steady_clock::now() < Deadline) {
      {
        std::unique_lock<std::mutex> Locker(MemInst.getWaiterMapMutex());
        size_t Waiting = 0;
        for (const auto &[Address, Queue] : MemInst.getWaiterMap()) {
          Waiting += Queue.size();
        }
        if (Waiting >= Count) {
          return true;
        }
      }
      std::this_thread::sleep_for(1ms);
    }
    return false;
  }

  AsyncT &start(std::string_view ModName, std::string_view Func,
                std::vector<WasmEdge::ValVariant> Params = {},
                std::vector<WasmEdge::ValType> ParamTypes = {}) {
    return Workers.emplace_back(
        ModName.empty() ? VM->asyncExecute(Func, Params, ParamTypes)
                        : VM->asyncExecute(ModName, Func, Params, ParamTypes));
  }

  AsyncT &startNotify(uint32_t Address, uint32_t Count) {
    return start({}, "notify"sv, {Address, Count},
                 {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                  WasmEdge::ValType(WasmEdge::TypeCode::I32)});
  }

  WasmEdge::Expect<uint32_t> notify(uint32_t Address, uint32_t Count) {
    return result(startNotify(Address, Count));
  }

  /// Register a module whose start function waits in the gate, so the VM
  /// holds its lock and no other run starts until openGate(). False when the
  /// start function does not reach the gate in time.
  bool closeGate() {
    auto GateFunc = std::make_unique<Gate>();
    auto Entered = GateFunc->Entered.get_future();
    GateFunc->Opened = Open.get_future().share();
    HostMod.addHostFunc("gate"sv, std::move(GateFunc));
    if (!VM->registerModule(HostMod)) {
      return false;
    }
    Locker = std::async(std::launch::async, [this]() {
      return VM->registerModule("gate"sv, GateWasm);
    });
    return Entered.wait_for(Bound) == std::future_status::ready;
  }

  /// Open the gate, and return the result of the module registration that
  /// waited in it.
  WasmEdge::Expect<void> openGate() {
    if (!GateOpened) {
      GateOpened = true;
      Open.set_value();
    }
    if (Locker.wait_for(Bound) != std::future_status::ready) {
      ADD_FAILURE() << "the registration did not finish in time";
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::Terminated);
    }
    return Locker.get();
  }

  /// Start a wait that times out long after Bound, so that a wait that a
  /// stop does not wake fails the test and still ends.
  AsyncT &startWait(uint32_t Address, std::string_view ModName = {}) {
    const auto Timeout = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(3 * Bound)
            .count());
    return start(ModName, "wait32"sv, {Address, UINT32_C(0), Timeout},
                 {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                  WasmEdge::ValType(WasmEdge::TypeCode::I32),
                  WasmEdge::ValType(WasmEdge::TypeCode::I64)});
  }

  /// The i32 that a run returns, or the error of the run.
  static WasmEdge::Expect<uint32_t>
  toU32(WasmEdge::Expect<
        std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>
            Result) {
    if (!Result) {
      return WasmEdge::Unexpect(Result.error());
    }
    if (Result->size() != 1 ||
        (*Result)[0].second.getCode() != WasmEdge::TypeCode::I32) {
      ADD_FAILURE() << "the run did not return one i32";
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::FuncSigMismatch);
    }
    return (*Result)[0].first.get<uint32_t>();
  }

  static WasmEdge::Expect<uint32_t> result(AsyncT &Worker) {
    if (!Worker.waitFor(Bound)) {
      ADD_FAILURE() << "the run did not finish in time";
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::Terminated);
    }
    return toU32(Worker.get());
  }

  static void expectInterrupted(AsyncT &Worker) {
    ASSERT_TRUE(Worker.waitFor(Bound));
    auto Result = Worker.get();
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }

  WasmEdge::Configure Conf;
  /// Host module of the gate. The VM uses it, so it outlives the VM.
  WasmEdge::Runtime::Instance::ModuleInstance HostMod{"host"sv};
  std::unique_ptr<WasmEdge::VM::VM> VM;
  std::list<AsyncT> Workers;
  std::promise<void> Open;
  bool GateOpened = false;
  std::future<WasmEdge::Expect<void>> Locker;
};

TEST_F(StopTest, StopDoesNotInterruptNextRun) {
  auto &Waiter = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 1));
  VM->stop();
  expectInterrupted(Waiter);
  auto Result = notify(4, 1);
  ASSERT_TRUE(Result);
  EXPECT_EQ(*Result, 0U);
}

TEST_F(StopTest, StopWakesWaiterAfterAnotherReturns) {
  auto &Waiter1 = startWait(0);
  auto &Waiter2 = startWait(8);
  ASSERT_TRUE(waitForWaiters({}, 2));
  auto Notified = notify(8, 1);
  ASSERT_TRUE(Notified);
  EXPECT_EQ(*Notified, 1U);
  auto Result = result(Waiter2);
  ASSERT_TRUE(Result);
  EXPECT_EQ(*Result, 0U);
  VM->stop();
  expectInterrupted(Waiter1);
}

TEST_F(StopTest, StopWakesWaitersOnEveryMemory) {
  auto &Waiter1 = startWait(0);
  auto &Waiter2 = startWait(0, Other);
  ASSERT_TRUE(waitForWaiters({}, 1));
  ASSERT_TRUE(waitForWaiters(Other, 1));
  VM->stop();
  expectInterrupted(Waiter1);
  expectInterrupted(Waiter2);
}

TEST_F(StopTest, StopInterruptsSpinningAndWaitingRuns) {
  auto &Spinner = start({}, "spin"sv);
  auto &Waiter = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 1));
  ASSERT_TRUE(waitForSpinner());
  VM->stop();
  expectInterrupted(Spinner);
  expectInterrupted(Waiter);
}

TEST_F(StopTest, StopReachesRunBeforeItStarts) {
  ASSERT_TRUE(closeGate());
  auto &Spinner = start({}, "spin"sv);
  VM->stop();
  auto Registered = openGate();
  EXPECT_TRUE(Registered ||
              Registered.error() == WasmEdge::ErrCode::Value::Interrupted);
  expectInterrupted(Spinner);
}

TEST_F(StopTest, CancelReachesRunBeforeItStarts) {
  ASSERT_TRUE(closeGate());
  auto &Spinner = start({}, "spin"sv);
  Spinner.cancel();
  EXPECT_TRUE(openGate());
  expectInterrupted(Spinner);
}

TEST_F(StopTest, CancelStopsOnlyItsRun) {
  auto &Waiter1 = startWait(0);
  auto &Waiter2 = startWait(8);
  ASSERT_TRUE(waitForWaiters({}, 2));
  Waiter1.cancel();
  expectInterrupted(Waiter1);
  auto Notified = notify(8, 1);
  ASSERT_TRUE(Notified);
  EXPECT_EQ(*Notified, 1U);
  auto Result = result(Waiter2);
  ASSERT_TRUE(Result);
  EXPECT_EQ(*Result, 0U);
}

/// A cancelled waiter does not take a notify, also before its thread wakes,
/// so the notify wakes the next waiter of the address.
TEST_F(StopTest, NotifySkipsCancelledWaiter) {
  auto &Waiter1 = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 1));
  auto &Waiter2 = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 2));
  Waiter1.cancel();
  auto Notified = notify(0, 1);
  ASSERT_TRUE(Notified);
  EXPECT_EQ(*Notified, 1U);
  auto Result = result(Waiter2);
  ASSERT_TRUE(Result);
  EXPECT_EQ(*Result, 0U);
  expectInterrupted(Waiter1);
}

TEST_F(StopTest, CancelOfFinishedRunKeepsOtherRuns) {
  auto &Waiter = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 1));
  auto &Finished = startNotify(4, 1);
  ASSERT_TRUE(result(Finished));
  Finished.cancel();
  auto Notified = notify(0, 1);
  ASSERT_TRUE(Notified);
  EXPECT_EQ(*Notified, 1U);
  auto Result = result(Waiter);
  ASSERT_TRUE(Result);
  EXPECT_EQ(*Result, 0U);
}

#ifdef WASMEDGE_USE_LLVM
/// The stop tests on JIT code that is not interruptible.
class StopTestJIT : public StopTest {
protected:
  void SetUp() override {
    Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::JIT);
    StopTest::SetUp();
  }
};

/// Code compiled without interruptible has no checkpoints of its own, but a
/// stop still ends its memory.atomic.wait in the runtime.
TEST_F(StopTestJIT, StopWakesCompiledWaiter) {
  ASSERT_FALSE(Conf.getCompilerConfigure().isInterruptible());
  ASSERT_TRUE(
      VM->getActiveModule()->findFuncExports("wait32"sv)->isCompiledFunction());
  auto &Waiter = startWait(0);
  ASSERT_TRUE(waitForWaiters({}, 1));
  VM->stop();
  expectInterrupted(Waiter);
}
#endif

TEST(AsyncExecute, ThreadTest) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(MersenneTwister19937));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    std::array<WasmEdge::Async<WasmEdge::Expect<std::vector<
                   std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>,
               4>
        AsyncResults;
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      AsyncResults[Index] =
          VM.asyncExecute("mt19937",
                          std::initializer_list<WasmEdge::ValVariant>{
                              UINT32_C(2504) * static_cast<uint32_t>(Index),
                              UINT64_C(5489), UINT64_C(100000) + Index},
                          {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    }
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      auto Result = AsyncResults[Index].get();
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I64);
      EXPECT_EQ((*Result)[0].first.get<uint64_t>(), Answers[Index]);
    }
  }
}

TEST(AsyncExecute, GasThreadTest) {
  WasmEdge::Configure Conf;
  Conf.getStatisticsConfigure().setInstructionCounting(true);
  Conf.getStatisticsConfigure().setCostMeasuring(true);
  Conf.getStatisticsConfigure().setTimeMeasuring(true);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(MersenneTwister19937));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    std::array<WasmEdge::Async<WasmEdge::Expect<std::vector<
                   std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>,
               4>
        AsyncResults;
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      AsyncResults[Index] =
          VM.asyncExecute("mt19937",
                          std::initializer_list<WasmEdge::ValVariant>{
                              UINT32_C(2504) * static_cast<uint32_t>(Index),
                              UINT64_C(5489), UINT64_C(100000) + Index},
                          {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    }
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      auto Result = AsyncResults[Index].get();
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I64);
      EXPECT_EQ((*Result)[0].first.get<uint64_t>(), Answers[Index]);
    }
  }
}

#ifdef WASMEDGE_USE_LLVM

TEST(AtomicWaitNotifyJIT, Memory32) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::JIT);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads32));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("notify");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("wait32");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("wait64");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    auto Result = VM.execute(
        "notify",
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(4), UINT32_C(1)},
        {WasmEdge::ValType(WasmEdge::TypeCode::I32),
         WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65533), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(2));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65533), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65532), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(65536), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
}

TEST(AtomicWaitNotifyJIT, Memory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::JIT);
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AtomicThreads64));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("notify");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst =
        VM.getActiveModule()->findFuncExports("notify-offset");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("wait32");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst =
        VM.getActiveModule()->findFuncExports("wait32-offset");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst = VM.getActiveModule()->findFuncExports("wait64");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    const auto *FuncInst =
        VM.getActiveModule()->findFuncExports("wait64-offset");
    ASSERT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isCompiledFunction());
  }
  {
    auto Result = VM.execute(
        "notify",
        std::initializer_list<WasmEdge::ValVariant>{UINT64_C(4), UINT32_C(1)},
        {WasmEdge::ValType(WasmEdge::TypeCode::I64),
         WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65533), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("notify",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(0));
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("notify-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(0xFFFFFFFFFFFFFFFC), UINT32_C(1)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(0), UINT32_C(0), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(2));
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65533), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait32",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait32-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait32-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT32_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I32),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result =
        VM.execute("wait32-offset",
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT64_C(0xFFFFFFFFFFFFFFFC), UINT32_C(1), UINT64_C(0)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I32),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65532), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnalignedAtomicAccess);
  }
  {
    auto Result = VM.execute("wait64",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65536), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result = VM.execute("wait64-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65520), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
  {
    auto Result = VM.execute("wait64-offset",
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT64_C(65528), UINT64_C(1), UINT64_C(0)},
                             {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64),
                              WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  {
    auto Result =
        VM.execute("wait64-offset",
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT64_C(0xFFFFFFFFFFFFFFF8), UINT64_C(1), UINT64_C(0)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64),
                    WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }
  auto *MemInst = VM.getActiveModule()->findMemoryExports("memory");
  ASSERT_NE(MemInst, nullptr);
  {
    std::unique_lock<std::mutex> Locker(MemInst->getWaiterMapMutex());
    auto &Queue = MemInst->getWaiterMap()[UINT64_C(8)];
    Queue.emplace_back();
    Queue.emplace_back();
  }
  {
    auto Result =
        VM.execute("notify-wrapped-count",
                   std::initializer_list<WasmEdge::ValVariant>{UINT64_C(8)},
                   {WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I32);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), UINT32_C(1));
  }
}

TEST(AOTAsyncExecute, ThreadTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      WasmEdge::CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  const auto Path = std::filesystem::temp_directory_path() /
                    WasmEdge::u8path("ThreadTest" WASMEDGE_LIB_EXTENSION);
  {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    ASSERT_TRUE(Compiler.checkConfigure());
    auto Module = *Loader.parseModule(MersenneTwister19937);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(MersenneTwister19937, std::move(*Data), Path));
  }

  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    std::array<WasmEdge::Async<WasmEdge::Expect<std::vector<
                   std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>,
               4>
        AsyncResults;
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      AsyncResults[Index] =
          VM.asyncExecute("mt19937",
                          std::initializer_list<WasmEdge::ValVariant>{
                              UINT32_C(2504) * static_cast<uint32_t>(Index),
                              UINT64_C(5489), UINT64_C(100000) + Index},
                          {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    }
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      auto Result = AsyncResults[Index].get();
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I64);
      EXPECT_EQ((*Result)[0].first.get<uint64_t>(), Answers[Index]);
    }
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AOTAsyncExecute, GasThreadTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getStatisticsConfigure().setInstructionCounting(true);
  Conf.getStatisticsConfigure().setCostMeasuring(true);
  Conf.getStatisticsConfigure().setTimeMeasuring(true);
  Conf.getCompilerConfigure().setOutputFormat(
      WasmEdge::CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  auto Path = std::filesystem::temp_directory_path() /
              WasmEdge::u8path("AOTGasTest" WASMEDGE_LIB_EXTENSION);

  {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    ASSERT_TRUE(Compiler.checkConfigure());
    auto Module = *Loader.parseModule(MersenneTwister19937);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(MersenneTwister19937, std::move(*Data), Path));
  }

  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    std::array<WasmEdge::Async<WasmEdge::Expect<std::vector<
                   std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>,
               4>
        AsyncResults;
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      AsyncResults[Index] =
          VM.asyncExecute("mt19937",
                          std::initializer_list<WasmEdge::ValVariant>{
                              UINT32_C(2504) * static_cast<uint32_t>(Index),
                              UINT64_C(5489), UINT64_C(100000) + Index},
                          {WasmEdge::ValType(WasmEdge::TypeCode::I32),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64),
                           WasmEdge::ValType(WasmEdge::TypeCode::I64)});
    }
    for (uint64_t Index = 0; Index < Answers.size(); ++Index) {
      auto Result = AsyncResults[Index].get();
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result)[0].second.getCode(), WasmEdge::TypeCode::I64);
      EXPECT_EQ((*Result)[0].first.get<uint64_t>(), Answers[Index]);
    }
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

#endif

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
