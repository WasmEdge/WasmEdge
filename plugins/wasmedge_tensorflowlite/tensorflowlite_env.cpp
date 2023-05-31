// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "tensorflowlite_env.h"
#include "tensorflowlite_module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeTensorflowLiteModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_tensorflowlite",
    .Description = "Tensorflow-Lite plug-in for WasmEdge.",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 13, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_tensorflowlite",
                .Description = "This module contains WasmEdge-TensorflowLite "
                               "host functions.",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasmEdgeTensorflowLite::TFLiteEnv::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
