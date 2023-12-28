#include "wasi_random/env.h"
#include "wasi_random/module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
createRandom(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiRandomModule;
}

Runtime::Instance::ModuleInstance *
createInsecure(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiRandomInsecureModule;
}

Runtime::Instance::ModuleInstance *
createInsecureSeed(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiRandomInsecureSeedModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_random",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 3,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi:random/random",
                .Description = "",
                .Create = createRandom,
            },
            {
                .Name = "wasi:random/insecure",
                .Description = "",
                .Create = createInsecure,
            },
            {
                .Name = "wasi:random/insecure-seed",
                .Description = "",
                .Create = createInsecureSeed,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasiRandomEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
