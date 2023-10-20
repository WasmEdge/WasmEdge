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
  addHostFunc("wasmedge_opencvmini_imencode",
              std::make_unique<WasmEdgeOpenCVMiniImencode>(Env));

  addHostFunc("wasmedge_opencvmini_imwrite",
              std::make_unique<WasmEdgeOpenCVMiniImwrite>(Env));

  addHostFunc("wasmedge_opencvmini_blur",
              std::make_unique<WasmEdgeOpenCVMiniBlur>(Env));
  addHostFunc("wasmedge_opencvmini_normalize",
              std::make_unique<WasmEdgeOpenCVMiniNormalize>(Env));
  addHostFunc("wasmedge_opencvmini_bilinear_sampling",
              std::make_unique<WasmEdgeOpenCVMiniBilinearSampling>(Env));
  addHostFunc("wasmedge_opencvmini_cvt_color",
              std::make_unique<WasmEdgeOpenCVMiniCvtColor>(Env));

  addHostFunc("wasmedge_opencvmini_rectangle",
              std::make_unique<WasmEdgeOpenCVMiniRectangle>(Env));

  addHostFunc("wasmedge_opencvmini_imshow",
              std::make_unique<WasmEdgeOpenCVMiniImshow>(Env));
  addHostFunc("wasmedge_opencvmini_waitkey",
              std::make_unique<WasmEdgeOpenCVMiniWaitKey>(Env));
}

} // namespace Host
} // namespace WasmEdge
