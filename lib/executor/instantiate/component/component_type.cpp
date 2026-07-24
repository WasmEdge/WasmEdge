// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreTypeSection &CoreTypeSec) {
  for (auto &Ty : CoreTypeSec.getContent()) {
    CompInst.addCoreType(Ty);
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::TypeSection &TypeSec) {
  for (auto &Ty : TypeSec.getContent()) {
    if (Ty.isResourceType()) {
      // Locally-defined resources mint their runtime identity here; the
      // destructor is a previously-defined core function.
      Runtime::Instance::FunctionInstance *Dtor = nullptr;
      if (auto DtorIdx = Ty.getResourceType().getDestructor()) {
        Dtor = CompInst.getCoreFunction(*DtorIdx);
      }
      CompInst.addResourceType(Ty, Dtor);
    } else {
      CompInst.addType(Ty);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
