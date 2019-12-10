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
namespace VM {

class Configure {
public:
  /// VM type enum class.
  enum class VMType : unsigned int { Wasm = 0, Ewasm, Wasi, ONNC };

  Configure() { Types.insert(VMType::Wasm); }
  ~Configure() = default;

  void addVMType(const VMType &Type) { Types.insert(Type); }

  bool checkIsVMType(const VMType &Type) const {
    return (Types.find(Type) != Types.end() ? true : false);
  }

  std::string &getStartFuncName() { return StartFuncName; }

  void setStartFuncName(const std::string &Name) { StartFuncName = Name; }

private:
  std::unordered_set<VMType> Types;
  std::string StartFuncName;
};

} // namespace VM
} // namespace SSVM
