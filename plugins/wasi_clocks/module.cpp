// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

using namespace AST::Component;

class Datetime : public RecordTy {
public:
  Datetime()
      : RecordTy{LabelValType("seconds", PrimValType::U64),
                 LabelValType("nanoseconds", PrimValType::U32)} {}
};

WallClockModule::WallClockModule()
    : ComponentInstance("wasi:clocks/wall-clock@0.2.0") {
  // FIXME: Here is a hard thing, `ResourceType&&` can move into
  // `ExportTypesMap`, but somehow `Datetime` here cannot.
  addHostType("datetime", Datetime());

  // TODO
  // now: func() -> datetime;
  // TODO
  // resolution: func() -> datetime;
}

} // namespace Host
} // namespace WasmEdge
