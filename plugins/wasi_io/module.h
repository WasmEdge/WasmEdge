// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"
#include "type.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiIOErrorModule : public Runtime::Instance::ComponentInstance {
public:
  WasiIOErrorModule();

  WasiIOEnvironment &getEnv() { return Env; }

private:
  WasiIOEnvironment Env;
};

class WasiIOStreamsModule : public Runtime::Instance::ComponentInstance {
public:
  WasiIOStreamsModule();

  WasiIOEnvironment &getEnv() { return Env; }

private:
  WasiIOEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
