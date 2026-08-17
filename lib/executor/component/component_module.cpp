// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate core module section. See executor.h.
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::CoreModuleSection &CoreModSec) {
  CompInst.addModule(CoreModSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
