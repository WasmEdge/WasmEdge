// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "sd_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class SDModule : public Runtime::Instance::ModuleInstance {
public:
  SDModule();
  StableDiffusion::SDEnviornment &getEnv() { return Env; }

private:
  StableDiffusion::SDEnviornment Env;
};

} // namespace Host
} // namespace WasmEdge
