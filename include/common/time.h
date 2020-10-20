// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>
#include <sys/time.h>
#include <unordered_map>

namespace SSVM {
namespace Support {

enum class TimerTag : uint32_t { Execution = 1U, HostFunc = 2U };

class TimeRecord {
public:
  void startRecord(const TimerTag TT) {
    startRecord(static_cast<uint32_t>(TT));
  }

  void startRecord(const uint32_t ID) {
    struct timeval TStart;
    gettimeofday(&TStart, NULL);
    StartTime.insert(
        std::pair{ID, (uint64_t)1000000 * TStart.tv_sec + TStart.tv_usec});
  }

  uint64_t stopRecord(const TimerTag TT) {
    return stopRecord(static_cast<uint32_t>(TT));
  }

  uint64_t stopRecord(const uint32_t ID) {
    std::unordered_map<uint32_t, uint64_t>::iterator It;
    uint64_t NDiff = 0;
    if ((It = StartTime.find(ID)) != StartTime.end()) {
      struct timeval TEnd;
      gettimeofday(&TEnd, NULL);
      NDiff = (uint64_t)1000000 * TEnd.tv_sec + TEnd.tv_usec - It->second;
      StartTime.erase(ID);
    }

    if (RecTime.find(ID) != RecTime.end()) {
      RecTime[ID] += NDiff;
    } else {
      RecTime.insert(std::make_pair(ID, NDiff));
    }
    return RecTime[ID];
  }

  void clearRecord(const TimerTag TT) {
    clearRecord(static_cast<uint32_t>(TT));
  }

  void clearRecord(const uint32_t ID) {
    StartTime.erase(ID);
    RecTime.erase(ID);
  }

  uint64_t getRecord(const TimerTag TT) {
    return getRecord(static_cast<uint32_t>(TT));
  }

  uint64_t getRecord(const uint32_t ID) {
    if (RecTime.find(ID) != RecTime.end()) {
      return RecTime[ID];
    }
    return 0;
  }

  void reset() {
    StartTime.clear();
    RecTime.clear();
  }

private:
  std::unordered_map<uint32_t, uint64_t> StartTime;
  std::unordered_map<uint32_t, uint64_t> RecTime;
};

} // namespace Support
} // namespace SSVM
