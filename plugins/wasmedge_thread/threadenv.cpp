// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadenv.h"
#include "threadmodule.h"

#include <thread>

namespace WasmEdge {
namespace Host {

void WasmEdgeThreadEnvironment::init(WasmEdge::VM::VM *VM) {
  WasmEdgeThreadEnvironment::VM = VM;
}

Expect<void> WasmEdgeThreadEnvironment::pthreadCreate(
    [[maybe_unused]] uint64_t *WasiThreadPtr, uint32_t WasiThreadFunc,
    uint32_t Arg) const {

  if (unlikely(!VM)) {
    return Unexpect(ErrCode::ExecutionFailed);
  } else {
    auto ThreadTunc = [&]() {
      VM->createThreadWithFunctionAddress(WasiThreadFunc, Arg);
    };
    [[maybe_unused]] pthread_t *ThreadPtr =
        static_cast<pthread_t *>(WasiThreadPtr);
    std::thread T(ThreadTunc);
    T.join();
  }

  return {};
}

Expect<void> WasmEdgeThreadEnvironment::pthreadJoin(uint64_t WasiThread,
                                                    void **WasiRetval) const {
  uint64_t Thread = static_cast<uint64_t>(WasiThread);
  pthread_join(Thread, WasiRetval);
  return {};
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
