// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ocr_module.h"
#include "ocr_func.h"

namespace WasmEdge {
namespace Host {

WasmEdgeOCRModule::WasmEdgeOCRModule() : ModuleInstance("wasmedge_ocr") {
  addHostFunc("num_of_extractions",
              std::make_unique<WasmEdgeOCR::NumOfExtractions>(Env));
  addHostFunc("get_output", std::make_unique<WasmEdgeOCR::GetOutput>(Env));
}

} // namespace Host
} // namespace WasmEdge
