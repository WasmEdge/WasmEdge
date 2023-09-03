#include "ffmpeg_env.h"
#include "avcodec/module.h"
#include "avformat/module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *createAVCodec(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpegAVCodecModule(WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *createAVDevice(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return nullptr;
}

Runtime::Instance::ModuleInstance *createAVFilter(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return nullptr;
}

Runtime::Instance::ModuleInstance *createAVFormat(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpegAVFormatModule(WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *createAVUtil(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return nullptr;
}

Runtime::Instance::ModuleInstance *createSWScale(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return nullptr;
}

Runtime::Instance::ModuleInstance *createSWResample(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return nullptr;
}

Plugin::Plugin::PluginDescriptor Descriptor {
  .Name = "",
  .Description = "",
  .APIVersion = Plugin::Plugin::CurrentAPIVersion,
  .Version = {0, 0, 0, 1} ,
  .ModuleCount = 7,
  .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
               .Name = "AVCodec",
               .Description = "encoding/decoding library",
               .Create = createAVCodec,
            },
            {
               .Name = "AVDevice" ,
               .Description = "special devices muxing/demuxing library",
               .Create = createAVDevice,
            },
            {
               .Name = "AVFilter",
               .Description = "graph-based frame editing library",
               .Create = createAVFilter,
            },
            {
                .Name = "AVFormat",
                .Description = "I/O and muxing/demuxing library",
                .Create = createAVFormat,
            },
            {
                .Name = "AVUtil",
                .Description = "common utility library",
                .Create = createAVUtil,
            },
            {
                .Name = "SWResample",
                .Description = "audio resampling, format conversion and mixing",
                .Create = createSWResample,
            },
            {
                .Name = "SWScale",
                .Description = "color conversion and scaling library",
                .Create = createSWScale,
            }
        },
  .AddOptions = nullptr,
};

}

Plugin::PluginRegister WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Register(&Descriptor);
std::weak_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Instance;
}
}