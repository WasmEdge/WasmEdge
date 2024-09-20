// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

// NOTE: How to have nice API here about resource type definition?
WasiIOErrorModule::WasiIOErrorModule()
    : ComponentInstance("wasi:io/error@0.2.0") {
  // FIXME: provide real resource type (and below)
  addExport("error", AST::Component::DefType{});
}

WasiIOStreamsModule::WasiIOStreamsModule()
    : ComponentInstance("wasi:io/streams@0.2.0") {
  addExport("input-stream", AST::Component::DefType{});
  addExport("output-stream", AST::Component::DefType{});
  addExport("error", AST::Component::DefType{});
}

} // namespace Host
} // namespace WasmEdge
