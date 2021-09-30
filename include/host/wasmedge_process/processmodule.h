// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasmedge_process/processenv.h"
#include "runtime/importobj.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessModule : public Runtime::ImportObject {
public:
  WasmEdgeProcessModule();

  WasmEdgeProcessEnvironment &getEnv() { return Env; }

private:
  WasmEdgeProcessEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
