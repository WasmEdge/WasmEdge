// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/spdlog.h"

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic push
// Suppression can be removed after spdlog with fix is released
// https://github.com/gabime/spdlog/pull/3198
#pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include <spdlog/sinks/callback_sink.h>
#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif
#ifdef _WIN32
#include <spdlog/sinks/wincolor_sink.h>
using color_sink_t = spdlog::sinks::wincolor_stdout_sink_mt;
#else
#include <spdlog/sinks/ansicolor_sink.h>
using color_sink_t = spdlog::sinks::ansicolor_stdout_sink_mt;
#endif

using namespace std::literals;

namespace WasmEdge {
namespace Log {

void setLogOff() { spdlog::set_level(spdlog::level::off); }

void setTraceLoggingLevel() { spdlog::set_level(spdlog::level::trace); }

void setDebugLoggingLevel() { spdlog::set_level(spdlog::level::debug); }

void setInfoLoggingLevel() { spdlog::set_level(spdlog::level::info); }

void setWarnLoggingLevel() { spdlog::set_level(spdlog::level::warn); }

void setErrorLoggingLevel() { spdlog::set_level(spdlog::level::err); }

void setCriticalLoggingLevel() { spdlog::set_level(spdlog::level::critical); }

bool setLoggingLevelFromString(std::string_view Level) {
  if (Level == "off"sv) {
    setLogOff();
    return true;
  }
  if (Level == "trace"sv) {
    setTraceLoggingLevel();
    return true;
  }
  if (Level == "debug"sv) {
    setDebugLoggingLevel();
    return true;
  }
  if (Level == "info"sv) {
    setInfoLoggingLevel();
    return true;
  }
  if (Level == "warning"sv || Level == "warn"sv) {
    setWarnLoggingLevel();
    return true;
  }
  if (Level == "error"sv) {
    setErrorLoggingLevel();
    return true;
  }
  if (Level == "fatal"sv || Level == "critical"sv) {
    setCriticalLoggingLevel();
    return true;
  }
  return false;
}

void setLoggingCallback(
    std::function<void(const spdlog::details::log_msg &)> Callback) {
  if (Callback) {
    auto Callback_sink =
        std::make_shared<spdlog::sinks::callback_sink_mt>(Callback);
    spdlog::set_default_logger(
        std::make_shared<spdlog::logger>("WasmEdge"s, Callback_sink));
  } else {
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        ""s, std::make_shared<color_sink_t>()));
  }
}

} // namespace Log
} // namespace WasmEdge
