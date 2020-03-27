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

#include <memory>
#include <string>
#include <unordered_set>

namespace SSVM {
namespace ExpVM {

class Configure {
public:
  /// VM type enum class.
  enum class VMType : uint8_t { Wasm = 0, Ewasm, Wasi, ONNC };

  Configure() { Types.insert(VMType::Wasm); }
  ~Configure() = default;

  void addVMType(const VMType Type) { Types.insert(Type); }

  void removeVMType(const VMType Type) { Types.erase(Type); }

  bool hasVMType(const VMType Type) const {
    return ((Types.find(Type) != Types.end()) ? true : false);
  }

private:
  std::unordered_set<VMType> Types;
};

} // namespace ExpVM
} // namespace SSVM
