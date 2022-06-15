// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "wasinnenv.h"

namespace WasmEdge {
namespace Host {

class WasiNNModule : public Runtime::Instance::ModuleInstance {
public:
  WasiNNModule();

private:
  WASINN::WasiNNEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
