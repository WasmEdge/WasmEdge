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
#include <memory>
#include <string>

namespace SSVM {
namespace VM {

class Configure {
public:
  /// VM type enum class.
  enum class VMType : uint8_t { Wasm = 0, Wasi, SSVM_Process, Max };

  Configure() { addVMType(VMType::Wasm); }
  ~Configure() = default;

  void addVMType(const VMType Type) { Types.set(uint8_t(Type)); }

  void removeVMType(const VMType Type) { Types.reset(uint8_t(Type)); }

  bool hasVMType(const VMType Type) const { return Types.test(uint8_t(Type)); }

private:
  std::bitset<uint8_t(VMType::Max)> Types;
};

} // namespace VM
} // namespace SSVM
