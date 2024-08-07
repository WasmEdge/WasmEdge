// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "sd_module.h"
#include "sd_func.h"

namespace WasmEdge {
namespace Host {

SDModule::SDModule() : ModuleInstance("wasmedge_stablediffusion") {
  addHostFunc("create_context",
              std::make_unique<StableDiffusion::SDCreateContext>(Env));
  addHostFunc("image_to_image",
              std::make_unique<StableDiffusion::SDImageToImage>(Env));
  addHostFunc("text_to_image",
              std::make_unique<StableDiffusion::SDTextToImage>(Env));
  addHostFunc("convert", std::make_unique<StableDiffusion::SDConvert>(Env));
}

} // namespace Host
} // namespace WasmEdge
