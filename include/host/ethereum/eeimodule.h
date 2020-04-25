// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "runtime/importobj.h"
#include "eeienv.h"

#include <cstdint>

namespace SSVM {
namespace Host {

class EEIModule : public Runtime::ImportObject {
public:
  EEIModule() = delete;
  EEIModule(uint64_t &CostLimit, uint64_t &CostSum);

  EVMEnvironment &getEnv() { return Env; }

private:
  EVMEnvironment Env;
};

} // namespace Host
} // namespace SSVM
