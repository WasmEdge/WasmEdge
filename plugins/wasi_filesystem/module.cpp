// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

TypesModule::TypesModule() : ComponentInstance("wasi:filesystem/types@0.2.0") {
  addExport("descriptor", AST::Component::DefType{});
}

PreopensModule::PreopensModule()
    : ComponentInstance("wasi:filesystem/preopens@0.2.0") {}

} // namespace Host
} // namespace WasmEdge
