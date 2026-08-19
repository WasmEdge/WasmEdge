// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Component::Executor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::CoreModuleSection &CoreModSec) {
  CompInst.addModule(CoreModSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
