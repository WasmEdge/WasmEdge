// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/spdlog.h"

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic push
// Suppression can be removed after spdlog with fix is released
// https://github.com/gabime/spdlog/pull/3198
#pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include "spdlog/sinks/callback_sink.h"
#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

namespace WasmEdge {
namespace Log {

void setLogOff() { spdlog::set_level(spdlog::level::off); }

void setTraceLoggingLevel() { spdlog::set_level(spdlog::level::trace); }

void setDebugLoggingLevel() { spdlog::set_level(spdlog::level::debug); }

void setInfoLoggingLevel() { spdlog::set_level(spdlog::level::info); }

void setWarnLoggingLevel() { spdlog::set_level(spdlog::level::warn); }

void setErrorLoggingLevel() { spdlog::set_level(spdlog::level::err); }

void setCriticalLoggingLevel() { spdlog::set_level(spdlog::level::critical); }

void setLoggingCallback(
    std::function<void(const spdlog::details::log_msg &)> Callback) {
  auto Callback_sink =
      std::make_shared<spdlog::sinks::callback_sink_mt>(Callback);
  spdlog::set_default_logger(
      std::make_shared<spdlog::logger>("WasmEdge", Callback_sink));
}

} // namespace Log
} // namespace WasmEdge
