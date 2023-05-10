// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadenv.h"
#include "threadmodule.h"

#include <thread>

namespace WasmEdge {
namespace Host {

Runtime::Instance::ModuleInstance *createWasiThreadsModule(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiThreadsModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_threads",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 0, 0, 1},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_threads",
                .Description = "",
                .Create = createWasiThreadsModule,
            },
        },
    .AddOptions = nullptr,
};

Plugin::PluginRegister WasiThreadsEnvironment::Register(&Descriptor);

Expect<wasmedge_tid_t>
WasiThreadsEnvironment::wasiThreadSpawn(Executor::Executor *Exec,
                                        Runtime::Instance::ModuleInstance *Mods,
                                        uint32_t ThreadStartArg) {

  auto Tid = Manager.allocate();
  auto Warper = [=]() -> void {
    std::vector<ValVariant> Params{Tid, ThreadStartArg};

    auto WasiThreadStartFunc = Mods->findFuncExports(WASI_ENTRY_POINT);

    if (WasiThreadStartFunc == nullptr) {
      spdlog::error("can not find wasi_thread_start");
      spdlog::error(ErrCode::Value::HostFuncError);
      // exit() will terminate whole program
      std::exit(int(ErrCode::Value::HostFuncError));
    }

    auto Res = Exec->invoke(*WasiThreadStartFunc, Params,
                            {ValType::I32, ValType::I32});

    if (Res) {
      spdlog::info("thread end good");
    } else {
      spdlog::error("thread {} exit with error code: {}", Tid, Res.error());
      std::exit(1);
    }
  };
  spdlog::warn("std::thread Thread(Warper)");
  std::thread Thread(Warper);
  Thread.detach();

  return {Tid};
}

} // namespace Host
} // namespace WasmEdge
