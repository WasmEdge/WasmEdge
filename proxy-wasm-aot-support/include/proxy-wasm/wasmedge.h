// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memory>

#include "include/proxy-wasm/wasm_vm.h"

namespace proxy_wasm {

// Create a WasmEdge VM with automatic AOT detection.
// If the WASM bytecode contains precompiled AOT code (universal WASM format),
// it will be used automatically for better performance.
std::unique_ptr<WasmVm> createWasmEdgeVm();

#if defined(WASMEDGE_USE_LLVM) || defined(PROXY_WASM_WASMEDGE_USE_AOT)
// Create a WasmEdge VM with AOT mode explicitly enabled.
// This will use AOT-compiled code if available in the precompiled section.
std::unique_ptr<WasmVm> createWasmEdgeVmWithAot();

// Create a WasmEdge VM in interpreter-only mode.
// This disables AOT compilation even if precompiled code is available.
std::unique_ptr<WasmVm> createWasmEdgeVmInterpreterOnly();
#endif

} // namespace proxy_wasm
