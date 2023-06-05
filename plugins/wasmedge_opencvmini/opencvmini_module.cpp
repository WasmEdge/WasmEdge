// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "opencvmini_module.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeOpenCVMiniModule::WasmEdgeOpenCVMiniModule()
    : ModuleInstance("wasmedge_process") {
  addHostFunc("wasmedge_opencvmini_imdecode",
              std::make_unique<WasmEdgeProcessSetProgName>(Env));
}

} // namespace Host
} // namespace WasmEdge
