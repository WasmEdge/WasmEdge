#pragma once

class Loader {
public:
  /// Error code enumeration class.
  enum class ErrCode : unsigned int {
    Success = 0,
    UndefinedError,
    InvalidPath,
    ReadError,
    EndOfFile,
    InvalidGrammar
  };
};