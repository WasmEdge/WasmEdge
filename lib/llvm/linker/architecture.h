// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "aot/version.h"
#include "linker/link_graph.h"

#include <cstdint>
#include <optional>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

constexpr std::optional<AOT::Architecture> architecture(Target Value) noexcept {
  switch (Value) {
  case Target::X86_64:
    return AOT::Architecture::X86_64;
  case Target::AArch64:
    return AOT::Architecture::AArch64;
  case Target::ARM:
    return AOT::Architecture::ARMv7;
  case Target::RISCV64:
    return AOT::Architecture::RISCV64;
  case Target::S390X:
    return AOT::Architecture::S390X;
  }
  return std::nullopt;
}

constexpr std::optional<Target> target(AOT::Architecture Value) noexcept {
  switch (Value) {
  case AOT::Architecture::X86_64:
    return Target::X86_64;
  case AOT::Architecture::AArch64:
    return Target::AArch64;
  case AOT::Architecture::ARMv7:
    return Target::ARM;
  case AOT::Architecture::RISCV64:
    return Target::RISCV64;
  case AOT::Architecture::S390X:
    return Target::S390X;
  case AOT::Architecture::Unsupported:
    return std::nullopt;
  }
  return std::nullopt;
}

constexpr uint64_t maximumPageSize(ObjectFormat Format, Target Value) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return Value == Target::ARM || Value == Target::AArch64 ? 65536 : 4096;
  case ObjectFormat::MachO:
    return Value == Target::AArch64 ? 16384 : 4096;
  case ObjectFormat::COFF:
    return 4096;
  }
  return 4096;
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
