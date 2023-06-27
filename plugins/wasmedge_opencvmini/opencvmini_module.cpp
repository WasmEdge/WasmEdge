// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "opencvmini_module.h"
#include "opencvmini_func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeOpenCVMiniModule::WasmEdgeOpenCVMiniModule()
    : ModuleInstance("wasmedge_opencvmini") {
  addHostFunc("wasmedge_opencvmini_imdecode",
              std::make_unique<WasmEdgeOpenCVMiniImdecode>(Env));
  addHostFunc("wasmedge_opencvmini_imshow",
              std::make_unique<WasmEdgeOpenCVMiniImshow>(Env));
  addHostFunc("wasmedge_opencvmini_waitkey",
              std::make_unique<WasmEdgeOpenCVMiniWaitKey>(Env));
  addHostFunc("wasmedge_opencvmini_blur",
              std::make_unique<WasmEdgeOpenCVMiniBlur>(Env));
  addHostFunc("wasmedge_opencvmini_imwrite",
              std::make_unique<WasmEdgeOpenCVMiniImwrite>(Env));
}

} // namespace Host
} // namespace WasmEdge
