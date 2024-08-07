// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "image_module.h"
#include "image_func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeImageModule::WasmEdgeImageModule()
    : Runtime::Instance::ModuleInstance("wasmedge_image") {
  addHostFunc("load_jpg", std::make_unique<WasmEdgeImage::LoadJPG>(Env));
  addHostFunc("load_png", std::make_unique<WasmEdgeImage::LoadPNG>(Env));
}

} // namespace Host
} // namespace WasmEdge
