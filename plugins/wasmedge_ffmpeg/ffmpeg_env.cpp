// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/module.h"
#include "avdevice/module.h"
#include "avfilter/module.h"
#include "avformat/module.h"
#include "avutil/module.h"
#include "swresample/module.h"
#include "swscale/module.h"

#include "ffmpeg_env.h"

namespace WasmEdge {
namespace Host {
namespace {

Runtime::Instance::ModuleInstance *
createAVCodec(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createAVDevice(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::AVDevice::WasmEdgeFFmpegAVDeviceModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createAVFilter(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::AVFilter::WasmEdgeFFmpegAVFilterModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createAVFormat(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::AVFormat::WasmEdgeFFmpegAVFormatModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createAVUtil(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createSWResample(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::SWResample::WasmEdgeFFmpegSWResampleModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Runtime::Instance::ModuleInstance *
createSWScale(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule(
      WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::getInstance());
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_ffmpeg",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 0, 0, 1},
    .ModuleCount = 7,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_ffmpeg_avcodec",
                .Description = "encoding/decoding library",
                .Create = createAVCodec,
            },
            {
                .Name = "wasmedge_ffmpeg_avdevice",
                .Description = "special devices muxing/demuxing library ",
                .Create = createAVDevice,
            },
            {
                .Name = "wasmedge_ffmpeg_avfilter",
                .Description = "graph-based frame editing library",
                .Create = createAVFilter,
            },
            {
                .Name = "wasmedge_ffmpeg_avformat",
                .Description = "I/O and muxing/demuxing library",
                .Create = createAVFormat,
            },
            {
                .Name = "wasmedge_ffmpeg_avutil",
                .Description = "utils utility library",
                .Create = createAVUtil,
            },
            {
                .Name = "wasmedge_ffmpeg_swresample",
                .Description = "audio resampling, format conversion and mixing",
                .Create = createSWResample,
            },
            {
                .Name = "wasmedge_ffmpeg_swscale",
                .Description = "color conversion and scaling library",
                .Create = createSWScale,
            }},
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace

std::weak_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv>
    WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Instance =
        std::make_shared<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv>();

std::shared_mutex WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Mutex;

} // namespace Host
} // namespace WasmEdge
