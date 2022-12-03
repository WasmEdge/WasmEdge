// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "opencvenv.h"

#include "imgcodecs/imgcodecsmodule.h"
#include "core/coremodule.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
createImgcodecs(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeOpenCvImgcodecsModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_opencv",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0,0, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_opencv_imgcodecs",
                .Description = "OpenCV imgcodecs module",
                .Create = createImgcodecs,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasmEdgeOpenCvEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
