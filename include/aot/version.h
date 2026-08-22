// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/aot/version.h - version definition -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the WasmEdge AOT binary version and serialized host
/// OS and architecture contract.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"

#include <cstdint>

namespace WasmEdge {
namespace AOT {

static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 3;

// The enum values below are serialized into AOT binaries; never renumber them.
enum class OSType : uint8_t {
  Unsupported = UINT8_MAX,
  Linux = 1,
  MacOS = 2,
  Windows = 3,
};

enum class SectionKind : uint8_t {
  Text = 1,
  Data = 2,
  BSS = 3,
  Unwind = 4,
};

enum class Architecture : uint8_t {
  Unsupported = UINT8_MAX,
  X86_64 = 1,
  AArch64 = 2,
  RISCV64 = 3,
  ARMv7 = 4,
  S390X = 5,
};

/// OS byte expected in AOT binaries for the current host.
#if WASMEDGE_OS_LINUX
static inline constexpr OSType kHostOSType = OSType::Linux;
#elif WASMEDGE_OS_MACOS
static inline constexpr OSType kHostOSType = OSType::MacOS;
#elif WASMEDGE_OS_WINDOWS
static inline constexpr OSType kHostOSType = OSType::Windows;
#else
static inline constexpr OSType kHostOSType = OSType::Unsupported;
#endif

/// Architecture byte expected in AOT binaries for the current host.
#if defined(__x86_64__) || defined(_M_X64)
static inline constexpr Architecture kHostArchitecture = Architecture::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
static inline constexpr Architecture kHostArchitecture = Architecture::AArch64;
#elif defined(__riscv) && __riscv_xlen == 64
static inline constexpr Architecture kHostArchitecture = Architecture::RISCV64;
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH == 7) ||          \
    (defined(_M_ARM) && _M_ARM == 7)
static inline constexpr Architecture kHostArchitecture = Architecture::ARMv7;
#elif defined(__s390x__)
static inline constexpr Architecture kHostArchitecture = Architecture::S390X;
#else
static inline constexpr Architecture kHostArchitecture =
    Architecture::Unsupported;
#endif

} // namespace AOT
} // namespace WasmEdge
