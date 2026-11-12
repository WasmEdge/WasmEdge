// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
