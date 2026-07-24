// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/host/wasi_preview2/wasip2.h ------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// WASI Preview 2 host interfaces built on the component model: each WIT
/// interface (wasi:clocks/monotonic-clock@0.2.0, wasi:random/random@0.2.0,
/// wasi:cli/*, wasi:io/*) becomes a host ComponentInstance registered in the
/// store by its interface name, exporting host component functions and
/// host-destructed resources.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/component/component.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASIPreview2 {

/// Build the supported wasi 0.2 interface instances. The caller owns the
/// returned instances and must keep them alive while registered.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
createInterfaces();

} // namespace WASIPreview2
} // namespace Host
} // namespace WasmEdge
