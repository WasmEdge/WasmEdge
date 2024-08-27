// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/spdlog.h - Logging system -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the linkage of logging system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/filesystem.h"
#include "common/int128.h"
#define SPDLOG_NO_EXCEPTIONS 1
#include "spdlog/spdlog.h"

namespace WasmEdge {
namespace Log {

void setLogOff();

void setDebugLoggingLevel();

void setInfoLoggingLevel();

void setWarnLoggingLevel();

void setErrorLoggingLevel();

} // namespace Log
} // namespace WasmEdge

template <>
struct fmt::formatter<std::filesystem::path>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator format(const std::filesystem::path &Path,
                                       fmt::format_context &Ctx) const {
    // mimic std::quoted
    constexpr const char Delimiter = '"';
    constexpr const char Escape = '\\';
    auto Quoted = fmt::memory_buffer();
    auto Iter = std::back_inserter(Quoted);
    *Iter++ = Delimiter;
    for (const auto C : Path.u8string()) {
      if (C == Delimiter || C == Escape) {
        *Iter++ = Escape;
      }
      *Iter++ = C;
    }
    *Iter++ = Delimiter;
    return fmt::formatter<std::string_view>::format(
        std::string_view(Quoted.data(), Quoted.size()), Ctx);
  }
};
