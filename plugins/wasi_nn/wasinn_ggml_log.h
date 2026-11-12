// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/spdlog.h"

#include <ggml.h>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// Shared body of the GGML, BitNet, and whisper log callbacks; callers check
// their enable gate first. Kept in a .cpp-only header because it needs
// <ggml.h>, which the broadly included type headers must stay free of.
inline void logGgmlMessage(ggml_log_level LogLevel, const char *LogText,
                           std::string_view Prefix) noexcept {
  using namespace std::literals;
  std::string Text(LogText);
  // Remove the trailing newlines.
  Text = Text.erase(Text.find_last_not_of("\n") + 1);
  // Skip for "."
  if (Text == ".") {
    return;
  }
  if (LogLevel == GGML_LOG_LEVEL_ERROR) {
    spdlog::error("[WASI-NN] {}: {}"sv, Prefix, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_WARN) {
    spdlog::warn("[WASI-NN] {}: {}"sv, Prefix, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_INFO) {
    spdlog::info("[WASI-NN] {}: {}"sv, Prefix, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_DEBUG) {
    spdlog::debug("[WASI-NN] {}: {}"sv, Prefix, Text);
  }
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
