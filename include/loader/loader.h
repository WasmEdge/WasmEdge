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

#include "module.h"
#include <string>

/// Loader flow control class.
class Loader {
public:
  /// Error code enumeration class.
  enum class ErrCode : unsigned int {
    Success = 0,
    /// Error code of loader flow
    WrongLoaderFlow,

    /// Error code of file input
    InvalidPath,
    ReadError,
    EndOfFile,

    /// Error code of parsing
    InvalidGrammar
  };

  Loader() = default;
  ~Loader() = default;

  /// Set the file path to loader.
  ErrCode setPath(const std::string &FilePath);

  /// Load and Parse the file into AST::Module.
  ErrCode parseModule();

  /// Validate AST::Module.
  ErrCode validateModule();

  /// Get the result AST::Module node.
  ErrCode getModule(std::unique_ptr<AST::Module> &OutModule);

private:
  /// Loader State
  enum class State : unsigned int {
    Inited,
    PathSet,
    Parsed,
    Validated,
    Finished
  };

  State Stat = State::Inited;
  std::unique_ptr<AST::Module> Mod;
  FileMgrFStream FMgr;
};