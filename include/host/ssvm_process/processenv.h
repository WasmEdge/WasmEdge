// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace SSVM {
namespace Host {

class SSVMProcessEnvironment {
public:
  /// Commands
  std::string Name;
  std::vector<std::string> Args;
  std::unordered_map<std::string, std::string> Envs;

  /// IO
  std::vector<uint8_t> StdIn;
  std::vector<uint8_t> StdOut;
  std::vector<uint8_t> StdErr;

  /// Configurations
  /// Default timeout in milliseconds.
  static inline const uint32_t DEFAULT_TIMEOUT = 10000;
  /// Default polling time in milliseconds.
  static inline const uint32_t DEFAULT_POLLTIME = 1;
  uint32_t TimeOut = DEFAULT_TIMEOUT; /// Timeout in milliseconds.

  /// Results
  uint32_t ExitCode = 0;
};

} // namespace Host
} // namespace SSVM
