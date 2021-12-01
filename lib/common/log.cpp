// SPDX-License-Identifier: Apache-2.0

#include "common/log.h"

namespace WasmEdge {
namespace Log {

void setDebugLoggingLevel() { spdlog::set_level(spdlog::level::debug); }

void setErrorLoggingLevel() { spdlog::set_level(spdlog::level::err); }

} // namespace Log
} // namespace WasmEdge
