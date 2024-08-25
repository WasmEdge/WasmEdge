// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/spdlog.h"

namespace WasmEdge {
namespace Log {

void setLogOff() { spdlog::set_level(spdlog::level::off); }

void setDebugLoggingLevel() { spdlog::set_level(spdlog::level::debug); }

void setInfoLoggingLevel() { spdlog::set_level(spdlog::level::info); }

void setWarnLoggingLevel() { spdlog::set_level(spdlog::level::warn); }

void setErrorLoggingLevel() { spdlog::set_level(spdlog::level::err); }

} // namespace Log
} // namespace WasmEdge
