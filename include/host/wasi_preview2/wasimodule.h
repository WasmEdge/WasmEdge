// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "host/wasi_preview2/environ.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiPreview2Module : public Runtime::Instance::ModuleInstance {
public:
  WasiPreview2Module();

  WASIPreview2::Environ &getEnv() noexcept { return Env; }
  const WASIPreview2::Environ &getEnv() const noexcept { return Env; }

private:
  WASIPreview2::Environ Env;
};

} // namespace Host
} // namespace WasmEdge
