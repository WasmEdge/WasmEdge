// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/common/timer.h - Timer class definition ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the timer class used by statistics.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "errcode.h"

#include <array>
#include <chrono>
#include <optional>
#include <string>

namespace WasmEdge {
namespace Timer {

enum class TimerTag : uint32_t { Wasm, HostFunc, Max };

class Timer {
public:
  using Clock = std::chrono::steady_clock;

  constexpr Timer() noexcept { reset(); }

  void startRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    const uint32_t Index = uint32_t(TT);
    StartTime[Index].emplace(Clock::now());
  }

  void stopRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    const uint32_t Index = uint32_t(TT);
    if (auto &Start = StartTime[Index]) {
      const auto Diff = Clock::now() - *Start;
      RecTime[Index] += Diff;
      Start.reset();
    }
  }

  void clearRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    const uint32_t Index = uint32_t(TT);
    StartTime[Index].reset();
    RecTime[Index] = Clock::duration::zero();
  }

  constexpr Clock::duration getRecord(const TimerTag TT) const noexcept {
    assuming(TT < TimerTag::Max);
    const uint32_t Index = uint32_t(TT);
    return RecTime[Index];
  }

  constexpr void reset() noexcept {
    for (auto &Start : StartTime) {
      Start.reset();
    }
    for (auto &Rec : RecTime) {
      Rec = Clock::duration::zero();
    }
  }

private:
  std::array<std::optional<Clock::time_point>, uint32_t(TimerTag::Max)>
      StartTime{};
  std::array<Clock::duration, uint32_t(TimerTag::Max)> RecTime{};
};

} // namespace Timer
} // namespace WasmEdge
