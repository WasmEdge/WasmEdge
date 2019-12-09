#pragma once

#include "support/time.h"
#include "vm/environment.h"

#include <string>
#include <sys/time.h>
#include <unordered_map>

namespace SSVM {
namespace VM {

class ONNCEnvironment : public Environment {
public:
  ONNCEnvironment() = default;
  virtual ~ONNCEnvironment() = default;

  virtual void clear() { Recorder.reset(); }

  void clear(const std::string &Key) { Recorder.clearRecord(Key); }

  void setStart(const std::string &Key) { Recorder.startRecord(Key); }

  uint64_t setStop(const std::string &Key) { return Recorder.stopRecord(Key); }

private:
  Support::TimeRecord Recorder;
};

} // namespace VM
} // namespace SSVM