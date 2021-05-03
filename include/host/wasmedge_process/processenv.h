// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessEnvironment {
public:
  /// Default timeout in milliseconds.
  static inline const uint32_t DEFAULT_TIMEOUT = 10000;
  /// Default polling time in milliseconds.
  static inline const uint32_t DEFAULT_POLLTIME = 1;

  /// Commands
  std::string Name;
  std::vector<std::string> Args;
  std::unordered_map<std::string, std::string> Envs;

  /// IO
  std::vector<uint8_t> StdIn;
  std::vector<uint8_t> StdOut;
  std::vector<uint8_t> StdErr;

  /// Configurations
  uint32_t TimeOut = DEFAULT_TIMEOUT;         /// Timeout in milliseconds.
  std::unordered_set<std::string> AllowedCmd; /// Programs in white list.
  bool AllowedAll = false;                    /// Flag to allow all programs.

  /// Results
  uint32_t ExitCode = 0;
};

} // namespace Host
} // namespace WasmEdge
