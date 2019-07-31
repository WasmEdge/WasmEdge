//===-- ssvm/vm/vm.h - VM execution flow class definition -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
///
///
//===----------------------------------------------------------------------===//
#pragma once

#include "loader/loader.h"
#include "result.h"
#include <cstdint>
#include <string>
#include <vector>

namespace SSVM {
namespace VM {

enum class ErrCode : unsigned int {
  Success = 0,
  Failed,
  Invalid
};

/// VM execution flow class
class VM {
public:
  VM() = default;
  ~VM() = default;

  /// Set the wasm file path.
  ErrCode setPath(const std::string &FilePath);
  /// Set the input data.
  ErrCode setInput(const std::vector<uint8_t> &InputVec);
  /// Execute wasm with given input.
  ErrCode execute();
  /// Return VMResult
  Result getResult() { return VMResult; }

private:
  Loader::Loader LoaderEngine;
  std::unique_ptr<AST::Module> Mod = nullptr;
  std::vector<uint8_t> Input;
  Result VMResult;
};
} // namespace VM
} // namespace SSVM
