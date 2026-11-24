// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_module.h"
#include "sd_func.h"

#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {

SDModule::SDModule(std::shared_ptr<StableDiffusion::Backend> B,
                   std::chrono::system_clock::time_point CreatedAt)
    : ModuleInstance("wasmedge_stablediffusion"), Env(std::move(B), CreatedAt) {
  addHostFunc("abi_version",
              std::make_unique<StableDiffusion::SDAbiVersion>(Env));
  addHostFunc("session_open",
              std::make_unique<StableDiffusion::SDSessionOpen>(Env));
  addHostFunc("session_close",
              std::make_unique<StableDiffusion::SDSessionClose>(Env));
  addHostFunc("generate", std::make_unique<StableDiffusion::SDGenerate>(Env));
  addHostFunc("result_info",
              std::make_unique<StableDiffusion::SDResultInfo>(Env));
  addHostFunc("result_read",
              std::make_unique<StableDiffusion::SDResultRead>(Env));
}

} // namespace Host
} // namespace WasmEdge
