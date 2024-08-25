// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasmedge_image_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeImageModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeImageModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_image") {
    addHostFunc("load_jpg", std::make_unique<WasmEdgeImageMock::LoadJPG>());
    addHostFunc("load_png", std::make_unique<WasmEdgeImageMock::LoadPNG>());
  }
};

} // namespace Host
} // namespace WasmEdge
