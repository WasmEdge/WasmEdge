// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {

class WasiHttpEnvironment {
public:
  WasiHttpEnvironment() noexcept;
  Runtime::Instance::ComponentInstance *getComponentInstance() {
    return Instance;
  }
  void setComponentInstance(Runtime::Instance::ComponentInstance *I) {
    Instance = I;
  }

private:
  Runtime::Instance::ComponentInstance *Instance;
};

} // namespace Host
} // namespace WasmEdge
