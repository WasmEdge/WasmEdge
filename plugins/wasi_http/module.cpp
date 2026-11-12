// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiHttpModule::WasiHttpModule() : ComponentInstance("wasi:http/test") {
  addHostFunc("http-get", std::make_unique<WasiHttpGet>(Env));
  addHostFunc("print", std::make_unique<WasiHttpPrint>(Env));
}

} // namespace Host
} // namespace WasmEdge
