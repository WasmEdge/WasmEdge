// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace WasmEdge {
namespace Timer {

enum class TimerTag : uint32_t { Wasm, HostFunc, Max };

class Timer {
public:
  using Clock = std::chrono::steady_clock;

  Timer() noexcept { unsafeReset(); }

  void startRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    std::unique_lock Lock(Mutex);
    const uint32_t Index = static_cast<uint32_t>(TT);
    StartTime[Index].emplace(std::this_thread::get_id(), Clock::now());
  }

  void stopRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    std::unique_lock Lock(Mutex);
    const uint32_t Index = static_cast<uint32_t>(TT);
    auto &Map = StartTime[Index];
    if (auto Iter = Map.find(std::this_thread::get_id()); Iter != Map.end()) {
      const auto Diff = Clock::now() - Iter->second;
      RecTime[Index] += Diff;
      Map.erase(Iter);
    }
  }

  void clearRecord(const TimerTag TT) noexcept {
    assuming(TT < TimerTag::Max);
    std::unique_lock Lock(Mutex);
    const uint32_t Index = static_cast<uint32_t>(TT);
    StartTime[Index].clear();
    RecTime[Index] = Clock::duration::zero();
  }

  Clock::duration getRecord(const TimerTag TT) const noexcept {
    assuming(TT < TimerTag::Max);
    std::shared_lock Lock(Mutex);
    const uint32_t Index = static_cast<uint32_t>(TT);
    return RecTime[Index];
  }

  void reset() noexcept {
    std::unique_lock Lock(Mutex);
    unsafeReset();
  }

private:
  void unsafeReset() noexcept {
    for (auto &Start : StartTime) {
      Start.clear();
    }
    for (auto &Rec : RecTime) {
      Rec = Clock::duration::zero();
    }
  }

  mutable std::shared_mutex Mutex;
  std::array<std::unordered_map<std::thread::id, Clock::time_point>,
             uint32_t(TimerTag::Max)>
      StartTime{};
  std::array<Clock::duration, uint32_t(TimerTag::Max)> RecTime{};
};

} // namespace Timer
} // namespace WasmEdge
