// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/compiler/compiler.h - Compiler class definition --------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Compiler class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "loader/loader.h"
#include "vm/envmgr.h"
#include "vm/configure.h"
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Compiler {

class Library;

/// Compiler class
class Compiler {
public:
  Compiler(VM::Configure &InputConfig) : Config(InputConfig), EnvMgr(Config) {}

  /// Set the wasm file path.
  ErrCode setPath(const std::string &FilePath) {
    WasmPath = FilePath;
    return ErrCode::Success;
  }

  /// Set the wasm byte code.
  ErrCode setCode(const std::vector<uint8_t> &Code) {
    WasmCode = Code;
    return ErrCode::Success;
  }

  /// Getter of Environment.
  template <typename T> T *getEnvironment(VM::Configure::VMType Type) {
    return EnvMgr.getEnvironment<T>(Type);
  }

  /// Compile codes
  ErrCode compile();

  /// Get compiled result
  Library &getLibrary();

  struct CompileContext;

private:
  /// Functions for running.
  ErrCode runLoader();

  /// Compile module
  ErrCode compile(const AST::Module &Module);
  ErrCode compile(const AST::ImportSection &ImportSection);
  ErrCode compile(const AST::ExportSection &ExportSection);
  ErrCode compile(const AST::TypeSection &TypeSection);
  ErrCode compile(const AST::GlobalSection &GlobalSection);
  ErrCode compile(const AST::MemorySection &MemorySection,
                  const AST::DataSection &DataSection);
  ErrCode compile(const AST::TableSection &TableSection,
                  const AST::ElementSection &ElementSection);
  ErrCode compile(const AST::FunctionSection &FunctionSection,
                  const AST::CodeSection &CodeSection);

  VM::Configure &Config;
  VM::EnvironmentManager EnvMgr;
  Loader::Loader LoaderEngine;
  std::string WasmPath;
  std::vector<uint8_t> WasmCode;
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<Library> Lib;
  CompileContext *Context = nullptr;
};

} // namespace Compiler
} // namespace SSVM
#include "library.h"
