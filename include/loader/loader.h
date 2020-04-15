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

#include "common/ast/module.h"
#include "common/errcode.h"

#include <string>
#include <vector>

namespace SSVM {
namespace Loader {

/// Loader flow control class.
class Loader {
public:
  Loader() = default;
  ~Loader() = default;

  /// Load data from file path.
  Expect<Bytes> loadFile(const std::string &FilePath);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>> parseModule(const std::string &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::vector<uint8_t> &Code);

private:
  FileMgrFStream FSMgr;
  FileMgrVector FVMgr;
  LDMgr LMgr;
};

} // namespace Loader
} // namespace SSVM
