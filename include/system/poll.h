// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/system/poll.h - Readiness wait on OS handles -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the readiness wait on OS handles.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"

#include <chrono>
#include <cstdint>
#include <optional>

namespace WasmEdge {

/// One OS handle to watch for reading or writing.
struct PollEntry {
  uint64_t Handle = 0;
  bool Write = false;
  bool Ready = false;
};

class Poll {
public:
  /// Wait until an entry is ready or Deadline passes; a past one does not wait.
  static void
  wait(Span<PollEntry> Entries,
       std::optional<std::chrono::steady_clock::time_point> Deadline) noexcept;
};

} // namespace WasmEdge
