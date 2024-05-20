// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace WasmEdge {
namespace Host {
namespace WASILogging {

class LogEnv {
public:
  LogEnv() noexcept {
    // TODO: Use the config in WasmEdge to set the logging level.
    StdoutLogger->set_level(spdlog::level::trace);
    StderrLogger->set_level(spdlog::level::trace);
    StdoutLogger->set_pattern(DefFormat);
    StderrLogger->set_pattern(DefFormat);
  }

  const std::shared_ptr<spdlog::logger> StdoutLogger =
      spdlog::stdout_color_mt("wasi_logging_stdout");
  const std::shared_ptr<spdlog::logger> StderrLogger =
      spdlog::stderr_color_mt("wasi_logging_stderr");
  const std::string DefFormat = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
  std::shared_ptr<spdlog::logger> FileLogger;
  std::string LogFileName;
};

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
