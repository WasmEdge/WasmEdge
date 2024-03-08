#pragma once

#include "plugin/plugin.h"
#include <spdlog/sinks/stdout_color_sinks.h>
namespace WasmEdge {
namespace Host {

class WasiLoggingEnvironment {
public:
  WasiLoggingEnvironment() noexcept {
    StdoutLogger->set_level(spdlog::level::trace);
    StderrLogger->set_level(spdlog::level::trace);
  }
  bool isCxtStrStderr = false;
  inline const static std::shared_ptr<spdlog::logger> StdoutLogger =
      spdlog::stdout_color_mt("wasi_logging_stdout");
  inline const static std::shared_ptr<spdlog::logger> StderrLogger =
      spdlog::stderr_color_mt("wasi_logging_stderr");
};

} // namespace Host
} // namespace WasmEdge
