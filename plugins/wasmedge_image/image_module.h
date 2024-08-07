// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "image_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeImageModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeImageModule();
  ~WasmEdgeImageModule() = default;

  WasmEdgeImage::ImgEnv &getEnv() { return Env; }

private:
  WasmEdgeImage::ImgEnv Env;
};

} // namespace Host
} // namespace WasmEdge
