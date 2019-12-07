#pragma once

#include "vm/environment.h"

#include <string>
#include <unordered_map>
#include <sys/time.h>

namespace SSVM {
namespace VM {

class ONNCTimer {
public:
  ONNCTimer() = default;
  virtual ~ONNCTimer() = default;

  virtual void clear() {
    IsRecording = false;
    RecTime = 0;
  }

  void start() {
    gettimeofday(&TStart, NULL);
    IsRecording = true;
  }

  uint64_t stop() {
    if (IsRecording) {
      struct timeval TEnd;
      gettimeofday(&TEnd, NULL);
      RecTime += (uint64_t)1000000 * (TEnd.tv_sec - TStart.tv_sec) +
                 TEnd.tv_usec - TStart.tv_usec;
      IsRecording = false;
    }
    return RecTime;
  }

private:
  bool IsRecording = false;
  struct timeval TStart;
  uint64_t RecTime = 0;
};

class ONNCEnvironment : public Environment {
public:
  ONNCEnvironment() = default;
  virtual ~ONNCEnvironment() = default;

  virtual void clear() {
    for(std::unordered_map<std::string, ONNCTimer>::iterator it = timers.begin(); it != timers.end(); ++it){
      it->second.clear();
    }
  }

  virtual void clear(std::string key) {
    timers[key].clear();
  }

  void setStart(std::string key) {
    timers[key].start();
  }

  uint64_t setStop(std::string key) {
    return timers.at(key).stop();
  }

private:
  std::unordered_map<std::string, ONNCTimer> timers;
};

} // namespace VM
} // namespace SSVM