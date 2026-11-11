// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

// BUILTIN-PLUGIN: Temporarily move the wasi-logging plugin sources here until
// the new plugin architecture is ready in 0.15.0.

#pragma once

#include "common/spdlog.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <string>

namespace WasmEdge {
namespace Host {
namespace WASILogging {

class LogEnv {
public:
  LogEnv() noexcept {
    using namespace std::literals;
    // Get the stdout and stderr logger.
    StdoutLogger = spdlog::get("wasi_logging_stdout"s);
    if (!StdoutLogger) {
      StdoutLogger = spdlog::stdout_color_mt("wasi_logging_stdout"s);
      StdoutLogger->set_level(spdlog::level::trace);
      StdoutLogger->set_pattern(DefFormat);
    }
    StderrLogger = spdlog::get("wasi_logging_stderr"s);
    if (!StderrLogger) {
      StderrLogger = spdlog::stderr_color_mt("wasi_logging_stderr"s);
      StderrLogger->set_level(spdlog::level::trace);
      StderrLogger->set_pattern(DefFormat);
    }
  }

  static const std::string DefFormat;
  std::shared_ptr<spdlog::logger> StdoutLogger;
  std::shared_ptr<spdlog::logger> StderrLogger;
  /// Whether the warning of a context file which cannot be opened is printed.
  std::atomic<bool> FileWarned{false};
};

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
