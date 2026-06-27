// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "runtime/instance/component/function.h"
#include "runtime/instance/component/hostfunc.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {
namespace Component {

FunctionInstance::~FunctionInstance() = default;

FunctionInstance::FunctionInstance(FunctionInstance &&Inst) noexcept
    : FuncType(std::move(Inst.FuncType)), Kind(Inst.Kind),
      CanonOpts(Inst.CanonOpts), LowerFunc(Inst.LowerFunc),
      HostFunc(std::move(Inst.HostFunc)) {}

FunctionInstance &
FunctionInstance::operator=(FunctionInstance &&Inst) noexcept {
  if (this != &Inst) {
    FuncType = std::move(Inst.FuncType);
    Kind = Inst.Kind;
    CanonOpts = Inst.CanonOpts;
    LowerFunc = Inst.LowerFunc;
    HostFunc = std::move(Inst.HostFunc);
  }
  return *this;
}

void HostFunctionDeleter::operator()(HostFunctionBase *Ptr) const { delete Ptr; }

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
