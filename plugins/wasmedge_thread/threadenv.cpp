// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadenv.h"
#include "threadmodule.h"

#include <thread>

namespace WasmEdge {
namespace Host {

Expect<wasmedge_tid_t>
WasmEdgeThreadEnvironment::threadCreate(Executor::Executor *Exec,
                                        uint32_t ThreadFunc, uint32_t Arg) {
  if (unlikely(!Exec)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  if (auto Ret = Exec->createThreadWithFunctionAddress(ThreadFunc, Arg);
      unlikely(!Ret)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  } else {
    std::function<void(void)> Func = *Ret;
    std::shared_ptr<std::thread> ThreadPtr =
        std::make_shared<std::thread>(Func);

    // ThreadPtr->join();
    return {Manager.allocate(ThreadPtr)};
  }
}

Expect<void>
WasmEdgeThreadEnvironment::threadJoin(wasmedge_tid_t Tid,
                                      [[maybe_unused]] void **WasiRetval) {
  if (auto ThreadPtr = Manager.get(Tid); unlikely(!ThreadPtr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  } else {
    ThreadPtr->join();
    Manager.free(Tid);
    return {};
  }
}

Runtime::Instance::ModuleInstance *create(void) noexcept {
  return new WasmEdgeThreadModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_thread",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_thread",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

Plugin::PluginRegister WasmEdgeThreadEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
