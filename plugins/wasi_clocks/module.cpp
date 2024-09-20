// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

// FIXME: provide real datetime
// record datetime {
//   seconds: u64,
//   nanoseconds: u32,
// }
WallClockModule::WallClockModule()
    : ComponentInstance("wasi:clocks/wall-clock@0.2.0") {
  addExport("datetime", AST::Component::DefType{});
}

} // namespace Host
} // namespace WasmEdge
