// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/loader/loader.h - Loader flow control class definition ---===//
//
// Part of the WasmEdge Project.
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

#include <string>
#include <vector>

namespace WasmEdge {
namespace Loader {

/// Loader flow control class.
class Loader {
public:
  Loader(const Configure &Conf,
         const AST::Module::IntrinsicsTable *IT = nullptr) noexcept
      : Conf(Conf), LMgr(IT), IntrinsicsTable(IT) {}
  ~Loader() noexcept = default;

  /// Load data from file path.
  Expect<std::vector<Byte>> loadFile(const std::filesystem::path &FilePath);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::filesystem::path &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>> parseModule(Span<const uint8_t> Code);

private:
  const Configure Conf;
  FileMgr FMgr;
  LDMgr LMgr;
  const AST::Module::IntrinsicsTable *IntrinsicsTable;
};

} // namespace Loader
} // namespace WasmEdge
