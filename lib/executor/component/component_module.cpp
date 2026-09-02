// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate core module section. See executor.h.
Expect<void> ComponentExecutor::instantiate(
    Component::Instantiator &Ctx,
    const AST::Component::CoreModuleSection &CoreModSec) {
  Ctx.getInstance().addModule(CoreModSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
