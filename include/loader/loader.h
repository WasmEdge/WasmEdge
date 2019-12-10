// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/loader/loader.h - Loader flow control class definition -------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Loader class, which controls flow
/// of WASM loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common.h"

#include <string>
#include <vector>

namespace SSVM {
namespace Loader {

/// Loader flow control class.
class Loader {
public:
  Loader() = default;
  ~Loader() = default;

  /// Set the file path to loader.
  ErrCode setPath(const std::string &FilePath);

  /// Set byte code to loader.
  ErrCode setCode(const std::vector<uint8_t> &Code);

  /// Load and Parse the file into AST::Module.
  ErrCode parseModule();

  /// Validate AST::Module.
  ErrCode validateModule();

  /// Get the result AST::Module node.
  ErrCode getModule(std::unique_ptr<AST::Module> &OutModule);

  /// Reset Loader.
  ErrCode reset(bool Force = false);

private:
  /// Loader State
  enum class State : unsigned int {
    Inited,
    PathSet_FStream,
    PathSet_Vector,
    Parsed,
    Validated,
    Finished
  };

  State Stat = State::Inited;
  std::unique_ptr<AST::Module> Mod;
  FileMgrFStream FSMgr;
  FileMgrVector FVMgr;
};

} // namespace Loader
} // namespace SSVM
