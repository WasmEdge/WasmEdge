//===-- ssvm/loader/common.h - Common definitions in Loader -----*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the common-use definitions in Loader.s
///
//===----------------------------------------------------------------------===//
#pragma once

namespace Loader {

/// Loader error code enum class.
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

} // namespace Loader