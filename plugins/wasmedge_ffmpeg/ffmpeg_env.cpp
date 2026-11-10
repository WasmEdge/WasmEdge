// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avcodec/module.h"
#include "avdevice/module.h"
#include "avfilter/avFilter.h"
#include "avfilter/module.h"
#include "avformat/module.h"
#include "avutil/module.h"
#include "swresample/module.h"
#include "swscale/module.h"

#include "ffmpeg_env.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <set>

namespace WasmEdge {
namespace Host {
namespace {

template <typename T> bool tagged(const void *Tag) {
  return Tag == WasmEdgeFFmpeg::detail::handleTag<T>();
}

// Releases the native object behind an owned handle that the guest never
// freed. The caller skips borrowed handles (children of another object and
// linked filter in/out nodes) and releases a pointer with several owned ids
// once; static descriptors such as codecs and formats match no case here.
void releaseHandle(const void *Tag, void *Ptr) {
  if (tagged<AVFrame>(Tag)) {
    auto *Frame = static_cast<AVFrame *>(Ptr);
    av_frame_free(&Frame);
  } else if (tagged<AVPacket>(Tag)) {
    auto *Packet = static_cast<AVPacket *>(Ptr);
    av_packet_free(&Packet);
  } else if (tagged<AVCodecContext>(Tag)) {
    auto *Ctx = static_cast<AVCodecContext *>(Ptr);
    avcodec_free_context(&Ctx);
  } else if (tagged<AVCodecParameters>(Tag)) {
    auto *Par = static_cast<AVCodecParameters *>(Ptr);
    avcodec_parameters_free(&Par);
  } else if (tagged<AVFormatContext>(Tag)) {
    auto *Ctx = static_cast<AVFormatContext *>(Ptr);
    if (Ctx->iformat != nullptr) {
      avformat_close_input(&Ctx);
    } else {
      avio_closep(&Ctx->pb);
      avformat_free_context(Ctx);
    }
  } else if (tagged<AVChapter>(Tag)) {
    auto *Chapter = static_cast<AVChapter *>(Ptr);
    av_dict_free(&Chapter->metadata);
    av_freep(&Chapter);
  } else if (tagged<AVDictionary>(Tag)) {
    auto **Holder = static_cast<AVDictionary **>(Ptr);
    av_dict_free(Holder);
    av_free(Holder);
  } else if (tagged<AVFilterGraph>(Tag)) {
    auto *Graph = static_cast<AVFilterGraph *>(Ptr);
    avfilter_graph_free(&Graph);
  } else if (tagged<AVFilterInOut>(Tag)) {
    auto *InOut = static_cast<AVFilterInOut *>(Ptr);
    avfilter_inout_free(&InOut);
  } else if (tagged<WasmEdgeFFmpeg::AVFilter::FilterPadView>(Tag)) {
    delete static_cast<WasmEdgeFFmpeg::AVFilter::FilterPadView *>(Ptr);
  } else if (tagged<SwsContext>(Tag)) {
    sws_freeContext(static_cast<SwsContext *>(Ptr));
  } else if (tagged<SwsFilter>(Tag)) {
    sws_freeFilter(static_cast<SwsFilter *>(Ptr));
  } else if (tagged<SwsVector>(Tag)) {
    sws_freeVec(static_cast<SwsVector *>(Ptr));
  } else if (tagged<SwrContext>(Tag)) {
    auto *Swr = static_cast<SwrContext *>(Ptr);
    swr_free(&Swr);
  } else if (tagged<AVDeviceInfoList>(Tag)) {
    auto **Holder = static_cast<AVDeviceInfoList **>(Ptr);
    avdevice_free_list_devices(Holder);
    av_free(Holder);
  } else if (tagged<uint8_t>(Tag)) {
    auto **Planes = static_cast<uint8_t **>(Ptr);
    av_freep(Planes);
    av_free(Planes);
  }
}

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

WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::~WasmEdgeFFmpegEnv() {
  std::set<void *> Released;
  for (const auto &Pair : FfmpegPtrMap) {
    if (isBorrowed(Pair.first) || !Released.insert(Pair.second.Ptr).second) {
      continue;
    }
    if (tagged<AVFormatContext>(Pair.second.Tag) && isMuxing(Pair.first)) {
      av_write_trailer(static_cast<AVFormatContext *>(Pair.second.Ptr));
    }
    releaseHandle(Pair.second.Tag, Pair.second.Ptr);
  }
}

std::weak_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv>
    WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Instance =
        std::make_shared<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv>();

std::shared_mutex WasmEdgeFFmpeg::WasmEdgeFFmpegEnv::Mutex;

} // namespace Host
} // namespace WasmEdge
