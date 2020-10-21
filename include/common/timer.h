// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>
#include <sys/time.h>
#include <unordered_map>

namespace SSVM {
namespace Timer {

enum class TimerTag : uint32_t { Wasm = 1U, HostFunc = 2U };

class Timer {
public:
  void startRecord(const TimerTag TT) {
    struct timeval TStart;
    gettimeofday(&TStart, NULL);
    StartTime.insert(
        std::pair{TT, (uint64_t)1000000 * TStart.tv_sec + TStart.tv_usec});
  }

  uint64_t stopRecord(const TimerTag TT) {
    std::unordered_map<TimerTag, uint64_t>::iterator It;
    uint64_t NDiff = 0;
    if ((It = StartTime.find(TT)) != StartTime.end()) {
      struct timeval TEnd;
      gettimeofday(&TEnd, NULL);
      NDiff = (uint64_t)1000000 * TEnd.tv_sec + TEnd.tv_usec - It->second;
      StartTime.erase(TT);
    }

    It = RecTime.find(TT);
    if (It != RecTime.end()) {
      NDiff += It->second;
      RecTime.insert(std::make_pair(TT, It->second + NDiff));
    }
    RecTime.insert(std::make_pair(TT, NDiff));
    return NDiff;
  }

  void clearRecord(const TimerTag TT) {
    StartTime.erase(TT);
    RecTime.erase(TT);
  }

  uint64_t getRecord(const TimerTag TT) const {
    auto It = RecTime.find(TT);
    if (It != RecTime.end()) {
      return It->second;
    }
    return 0;
  }

  void reset() {
    StartTime.clear();
    RecTime.clear();
  }

private:
  std::unordered_map<TimerTag, uint64_t> StartTime;
  std::unordered_map<TimerTag, uint64_t> RecTime;
};

} // namespace Timer
} // namespace SSVM
