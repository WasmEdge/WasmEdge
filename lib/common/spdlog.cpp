// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/spdlog.h"

#include <mutex>
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

namespace {
std::once_flag InitOnce;
}

void ensureInitialized() {
  std::call_once(InitOnce, []() {
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "WasmEdge"s, std::make_shared<color_sink_t>()));
    spdlog::set_level(spdlog::level::err);
  });
}

void setLogOff() { spdlog::set_level(spdlog::level::off); }

void setTraceLoggingLevel() { spdlog::set_level(spdlog::level::trace); }

void setDebugLoggingLevel() { spdlog::set_level(spdlog::level::debug); }

void setInfoLoggingLevel() { spdlog::set_level(spdlog::level::info); }

void setWarnLoggingLevel() { spdlog::set_level(spdlog::level::warn); }

void setErrorLoggingLevel() { spdlog::set_level(spdlog::level::err); }

void setCriticalLoggingLevel() { spdlog::set_level(spdlog::level::critical); }

void setLoggingCallback(
    std::function<void(const spdlog::details::log_msg &)> Callback) {
  std::call_once(InitOnce, []() {});
  if (Callback) {
    auto Callback_sink =
        std::make_shared<spdlog::sinks::callback_sink_mt>(Callback);
    spdlog::set_default_logger(
        std::make_shared<spdlog::logger>("WasmEdge"s, Callback_sink));
  } else {
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "WasmEdge"s, std::make_shared<color_sink_t>()));
  }
}

} // namespace Log
} // namespace WasmEdge
