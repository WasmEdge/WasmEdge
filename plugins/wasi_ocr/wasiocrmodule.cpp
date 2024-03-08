// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#include "wasiocrmodule.h"
#include "wasiocrfunc.h"

namespace WasmEdge {
namespace Host {

WasiOCRModule::WasiOCRModule() : ModuleInstance("wasi_ephemeral_ocr") {
  addHostFunc("num_of_extractions",
              std::make_unique<WasiOCRNumOfExtractions>(Env));
  addHostFunc("get_output", std::make_unique<WasiOCRGetOutput>(Env));
}

} // namespace Host
} // namespace WasmEdge
