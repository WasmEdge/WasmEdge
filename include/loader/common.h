//===-- ssvm/loader/common.h - Common definitions in Loader -----*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the common-use definitions in Loader.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {
namespace Loader {

/// Loader error code enum class.
enum class ErrCode : unsigned int {
  Success = 0,
  WrongLoaderFlow, /// Wrong loader's flow
  InvalidPath,     /// File not found
  ReadError,       /// Error when reading
  EndOfFile,       /// Reach end of file when reading
  InvalidGrammar   /// Parsing error
};

} // namespace Loader
} // namespace SSVM