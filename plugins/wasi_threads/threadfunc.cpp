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
                                      uint32_t ThreadStartArg) {
  // TODO: do this check on initialize
  spdlog::warn("WasiThreadSpawn body");
  if (!HasWasiEntryPoint(Frame.getModule())) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Executor is thread_local variable, do not copy it
  auto NewThreadExec = Frame.getExecutor();
  auto NewThreadModule = Frame.getModule()->Clone();

  return Env.wasiThreadSpawn(NewThreadExec, NewThreadModule, ThreadStartArg);
}

} // namespace Host
} // namespace WasmEdge
