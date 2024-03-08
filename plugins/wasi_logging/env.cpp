#include "wasi_logging/env.h"
#include "wasi_logging/module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiLoggingModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_logging",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi:logging/logging",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge