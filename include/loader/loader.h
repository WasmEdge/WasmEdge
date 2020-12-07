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
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/proposal.h"

#include <string>
#include <vector>

namespace SSVM {
namespace Loader {

/// Loader flow control class.
class Loader {
public:
  Loader(const ProposalConfigure &PConf) : PConf(PConf) {}
  ~Loader() = default;

  /// Load data from file path.
  Expect<std::vector<Byte>> loadFile(const std::filesystem::path &FilePath);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::filesystem::path &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>> parseModule(Span<const uint8_t> Code);

private:
  const ProposalConfigure &PConf;
  FileMgrFStream FSMgr;
  FileMgrVector FVMgr;
  LDMgr LMgr;
};

} // namespace Loader
} // namespace SSVM
