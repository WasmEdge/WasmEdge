#pragma once

#include "vm/environment.h"

#include <sys/time.h>

namespace SSVM {
namespace VM {

class ONNCEnvironment : public Environment {
public:
  ONNCEnvironment() = default;
  virtual ~ONNCEnvironment() = default;

  virtual void clear() {
    IsRecording = false;
    RecTime = 0;
  }

  void setStart() {
    gettimeofday(&TStart, NULL);
    IsRecording = true;
  }

  uint64_t setStop() {
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

} // namespace VM
} // namespace SSVM