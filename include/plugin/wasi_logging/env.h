// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#pragma once

#include "common/hexstr.h"
#include "common/spdlog.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>

namespace WasmEdge {
namespace Host {
namespace WASILogging {

class LogEnv {
public:
  LogEnv() noexcept {
    // Get the stdout and stderr logger.
    StdoutLogger = spdlog::get("wasi_logging_stdout");
    if (!StdoutLogger) {
      StdoutLogger = spdlog::stdout_color_mt("wasi_logging_stdout");
      StdoutLogger->set_level(spdlog::level::trace);
      StdoutLogger->set_pattern(DefFormat);
    }
    StderrLogger = spdlog::get("wasi_logging_stderr");
    if (!StderrLogger) {
      StderrLogger = spdlog::stderr_color_mt("wasi_logging_stderr");
      StderrLogger->set_level(spdlog::level::trace);
      StderrLogger->set_pattern(DefFormat);
    }

    std::random_device RandDev;
    std::mt19937 RandGen(RandDev());
    std::uniform_int_distribution<uint64_t> RandDist(
        0, std::numeric_limits<uint64_t>::max());

    std::unique_lock Lock(Mutex);
    do {
      InstanceID = RandDist(RandGen);
    } while (RegisteredID.find(InstanceID) != RegisteredID.cend());
    LogRegName = "wasi_logging_file_" + convertUIntToHexStr(InstanceID);
    RegisteredID.insert(InstanceID);
  }

  ~LogEnv() noexcept {
    std::unique_lock Lock(Mutex);
    spdlog::drop(LogFileName);
    RegisteredID.erase(InstanceID);
  }

  std::string_view getLogFileName() const noexcept { return LogFileName; }
  void setLogFileName(std::string_view Name) noexcept {
    LogFileName = std::string(Name);
  }

  const std::string &getLogRegName() const noexcept { return LogRegName; }

  uint64_t getInstanceID() const noexcept { return InstanceID; }

  static std::mutex Mutex;
  static std::unordered_set<uint64_t> RegisteredID;
  static const std::string DefFormat;
  std::shared_ptr<spdlog::logger> StdoutLogger;
  std::shared_ptr<spdlog::logger> StderrLogger;
  std::shared_ptr<spdlog::logger> FileLogger;

private:
  std::string LogFileName;
  std::string LogRegName;
  uint64_t InstanceID;
};

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
