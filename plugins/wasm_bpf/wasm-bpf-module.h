// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmBpfModule : public Runtime::Instance::ModuleInstance {
public:
  WasmBpfModule();
};
} // namespace Host
} // namespace WasmEdge
