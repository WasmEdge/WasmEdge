// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadfunc.h"
#include "common/log.h"

#include <vector>

namespace WasmEdge {
namespace Host {

static bool HasWasiEntryPoint(const Runtime::Instance::ModuleInstance *Mods) {
  if (auto Func = Mods->findFuncExports(WASI_ENTRY_POINT); Func != nullptr) {
    auto ParamTys = Func->getFuncType().getParamTypes();
    auto RetTy = Func->getFuncType().getReturnTypes();

    static const std::vector<ValType> Symbol{ValType::I32, ValType::I32};
    if (ParamTys.size() == 2 && ParamTys == Symbol && RetTy.size() == 0) {
      return true;
    }
  }

  return false;
}

Expect<int32_t> WasiThreadSpawn::body(const Runtime::CallingFrame &Frame,
                                      int32_t ThreadStartArg) {
  if (!HasWasiEntryPoint(Frame.getModule())) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Executor is thread_local variable, do not copy it
  auto NewThreadExec = Frame.getExecutor();
  auto NewThreadModule = Frame.getModule()->UnsafeCloneMemoryInEnv();

  if (!NewThreadModule ||
      NewThreadModule->findMemoryExports("memory") == nullptr) {
    spdlog::info("Can not find shared memory");
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.wasiThreadSpawn(NewThreadExec, std::move(NewThreadModule),
                             ThreadStartArg);
}

} // namespace Host
} // namespace WasmEdge
