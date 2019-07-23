#pragma once

namespace Loader {

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