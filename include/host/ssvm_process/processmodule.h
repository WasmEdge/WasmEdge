// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "processenv.h"
#include "runtime/importobj.h"

#include <cstdint>

namespace SSVM {
namespace Host {

class SSVMProcessModule : public Runtime::ImportObject {
public:
  SSVMProcessModule();

  SSVMProcessEnvironment &getEnv() { return Env; }

private:
  SSVMProcessEnvironment Env;
};

} // namespace Host
} // namespace SSVM
