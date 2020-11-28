// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/configure.h - consiguration class of ssvm -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the configuration class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <bitset>
#include <cstdint>

namespace SSVM {
namespace VM {

class Configure {
public:
  /// VM type enum class.
  enum class VMType : uint8_t { Wasm = 0, Wasi, SSVM_Process, Max };

  Configure() noexcept { addVMType(VMType::Wasm); }

  void addVMType(const VMType Type) noexcept {
    Types.set(static_cast<uint8_t>(Type));
  }

  void removeVMType(const VMType Type) noexcept {
    Types.reset(static_cast<uint8_t>(Type));
  }

  bool hasVMType(const VMType Type) const noexcept {
    return Types.test(static_cast<uint8_t>(Type));
  }

private:
  std::bitset<static_cast<uint8_t>(VMType::Max)> Types;
};

} // namespace VM
} // namespace SSVM
