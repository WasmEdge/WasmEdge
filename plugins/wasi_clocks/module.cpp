// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

using namespace AST::Component;

WallClockModule::WallClockModule()
    : ComponentInstance("wasi:clocks/wall-clock@0.2.0") {
  addExport("datetime",
            Record({LabelValType("seconds", PrimValType::U64),
                    LabelValType("nanoseconds", PrimValType::U32)}));
}

} // namespace Host
} // namespace WasmEdge
