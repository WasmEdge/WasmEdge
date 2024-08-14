// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "image_env.h"
#include "image_module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeImageModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_image",
    .Description = "Image loading plug-in for WasmEdge.",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 13, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_image",
                .Description =
                    "This module contains WasmEdge-Image host functions.",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
