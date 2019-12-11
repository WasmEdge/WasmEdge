// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>
#include <sys/time.h>
#include <unordered_map>

namespace SSVM {
namespace Support {

class TimeRecord {
public:
  void startRecord(const std::string &Name) {
    struct timeval TStart;
    gettimeofday(&TStart, NULL);
    StartTime.insert(std::make_pair(Name, (uint64_t)1000000 * TStart.tv_sec +
                                              TStart.tv_usec));
  }

  uint64_t stopRecord(const std::string &Name) {
    std::unordered_map<std::string, uint64_t>::iterator It;
    uint64_t NDiff = 0;
    if ((It = StartTime.find(Name)) != StartTime.end()) {
      struct timeval TEnd;
      gettimeofday(&TEnd, NULL);
      NDiff = (uint64_t)1000000 * TEnd.tv_sec + TEnd.tv_usec - It->second;
      StartTime.erase(Name);
    }

    if (RecTime.find(Name) != RecTime.end()) {
      RecTime[Name] += NDiff;
    } else {
      RecTime.insert(std::make_pair(Name, NDiff));
    }
    return RecTime[Name];
  }

  void clearRecord(const std::string &Name) {
    StartTime.erase(Name);
    RecTime.erase(Name);
  }

  uint64_t getRecord(const std::string &Name) {
    if (RecTime.find(Name) != RecTime.end()) {
      return RecTime[Name];
    }
    return 0;
  }

  void reset() {
    StartTime.clear();
    RecTime.clear();
  }

private:
  std::unordered_map<std::string, uint64_t> StartTime;
  std::unordered_map<std::string, uint64_t> RecTime;
};

} // namespace Support
} // namespace SSVM
