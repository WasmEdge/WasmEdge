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
/// architecture contract.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace WasmEdge {
namespace AOT {

static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 3;

enum class Architecture : uint8_t {
  Unsupported = UINT8_MAX,
  X86_64 = 1,
  AArch64 = 2,
  RISCV64 = 3,
  ARMv7 = 4,
  S390X = 5,
};

static_assert(static_cast<uint8_t>(Architecture::X86_64) == 1);
static_assert(static_cast<uint8_t>(Architecture::AArch64) == 2);
static_assert(static_cast<uint8_t>(Architecture::RISCV64) == 3);
static_assert(static_cast<uint8_t>(Architecture::ARMv7) == 4);
static_assert(static_cast<uint8_t>(Architecture::S390X) == 5);
static_assert(static_cast<uint8_t>(Architecture::Unsupported) == UINT8_MAX);

namespace Internal {

enum class HostArchitecture {
  Unsupported,
  X86_64,
  AArch64,
  ARM,
  RISCV64,
  S390X,
};

constexpr Architecture architecture(HostArchitecture Host,
                                    uint32_t ARMLevel = 0) noexcept {
  switch (Host) {
  case HostArchitecture::X86_64:
    return Architecture::X86_64;
  case HostArchitecture::AArch64:
    return Architecture::AArch64;
  case HostArchitecture::ARM:
    return ARMLevel == 7 ? Architecture::ARMv7 : Architecture::Unsupported;
  case HostArchitecture::RISCV64:
    return Architecture::RISCV64;
  case HostArchitecture::S390X:
    return Architecture::S390X;
  case HostArchitecture::Unsupported:
    return Architecture::Unsupported;
  }
  return Architecture::Unsupported;
}

static_assert(architecture(HostArchitecture::X86_64) == Architecture::X86_64);
static_assert(architecture(HostArchitecture::AArch64) == Architecture::AArch64);
static_assert(architecture(HostArchitecture::ARM, 7) == Architecture::ARMv7);
static_assert(architecture(HostArchitecture::ARM, 6) ==
              Architecture::Unsupported);
static_assert(architecture(HostArchitecture::ARM, 8) ==
              Architecture::Unsupported);
static_assert(architecture(HostArchitecture::RISCV64) == Architecture::RISCV64);
static_assert(architecture(HostArchitecture::S390X) == Architecture::S390X);
static_assert(architecture(HostArchitecture::Unsupported) ==
              Architecture::Unsupported);

} // namespace Internal

/// Architecture byte expected in AOT binaries for the current host.
#if defined(__x86_64__) || defined(_M_X64)
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::X86_64);
#elif defined(__aarch64__) || defined(_M_ARM64)
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::AArch64);
#elif defined(__riscv) && __riscv_xlen == 64
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::RISCV64);
#elif defined(__arm__) && defined(__ARM_ARCH)
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::ARM, __ARM_ARCH);
#elif defined(_M_ARM)
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::ARM, _M_ARM);
#elif defined(__s390x__)
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::S390X);
#else
static inline constexpr Architecture kHostArchitecture =
    Internal::architecture(Internal::HostArchitecture::Unsupported);
#endif

} // namespace AOT
} // namespace WasmEdge
