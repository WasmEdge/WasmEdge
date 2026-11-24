// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_backend.h"
#include "sd_env.h"

#include "runtime/instance/module.h"

#include <chrono>
#include <memory>

namespace WasmEdge {
namespace Host {

class SDModule : public Runtime::Instance::ModuleInstance {
public:
  // The plug-in passes only B. CreatedAt is then the creation time of the
  // module, before any guest can call it.
  explicit SDModule(std::shared_ptr<StableDiffusion::Backend> B,
                    std::chrono::system_clock::time_point CreatedAt =
                        std::chrono::system_clock::now());
  StableDiffusion::SDEnvironment &getEnv() noexcept { return Env; }

private:
  StableDiffusion::SDEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
