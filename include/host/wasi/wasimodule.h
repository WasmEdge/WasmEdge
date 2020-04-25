// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "runtime/importobj.h"
#include "wasienv.h"

#include <cstdint>

namespace SSVM {
namespace Host {

class WasiModule : public Runtime::ImportObject {
public:
  WasiModule();

  WasiEnvironment &getEnv() { return Env; }

private:
  WasiEnvironment Env;
};

} // namespace Host
} // namespace SSVM
