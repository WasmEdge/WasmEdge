#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class EEI : public HostFunction {
public:
  EEI(std::vector<unsigned char> &CallData) : Data(CallData) {}
  EEI() = delete;
  virtual ~EEI() = default;

protected:
  std::vector<unsigned char> &Data;
};

} // namespace Executor
} // namespace SSVM