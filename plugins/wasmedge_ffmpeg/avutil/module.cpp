#include "module.h"
#include "avDictionary.h"
#include "avFrame.h"
#include "avRational.h"
#include "avTime.h"
#include "avutil_func.h"
#include "channel_layout.h"
#include "error.h"
#include "pixfmt.h"
#include "samplefmt.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

WasmEdgeFFmpegAVUtilModule::WasmEdgeFFmpegAVUtilModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avutil") {

  // error.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_strerror",
              std::make_unique<AVUtilAVStrError>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_AVERROR",
              std::make_unique<AVUtilAVError>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_AVUNERROR",
              std::make_unique<AVUtilAVUNError>(Env));

  // rational.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_add_q", std::make_unique<AVAddQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_sub_q", std::make_unique<AVSubQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_mul_q", std::make_unique<AVMulQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_div_q", std::make_unique<AVDivQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_d2q", std::make_unique<AVD2Q>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_q2d", std::make_unique<AVQ2d>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_inv_q", std::make_unique<AVInvQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_q2intfloat",
              std::make_unique<AVQ2IntFloat>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_nearer_q",
              std::make_unique<AVNearerQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_cmp_q", std::make_unique<AVCmpQ>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_reduce",
              std::make_unique<AVReduce>(Env));

  // frame.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_alloc",
              std::make_unique<AVFrameAlloc>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_free",
              std::make_unique<AVFrameFree>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_width",
              std::make_unique<AVFrameWidth>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_height",
              std::make_unique<AVFrameHeight>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_width",
              std::make_unique<AVFrameSetWidth>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_height",
              std::make_unique<AVFrameSetHeight>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_video_format",
              std::make_unique<AVFrameVideoFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
              std::make_unique<AVFrameSetVideoFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_isnull",
              std::make_unique<AVFrameIsNull>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_linesize",
              std::make_unique<AVFrameLinesize>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_data",
              std::make_unique<AVFrameData>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_get_buffer",
              std::make_unique<AVFrameGetBuffer>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_audio_format",
              std::make_unique<AVFrameAudioFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
              std::make_unique<AVFrameSetAudioFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
              std::make_unique<AVFrameSetNbSamples>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
              std::make_unique<AVFrameSetChannelLayout>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_nb_samples",
              std::make_unique<AVFrameNbSamples>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_sample_rate",
              std::make_unique<AVFrameSampleRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_sample_rate",
              std::make_unique<AVFrameSetSampleRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_channels",
              std::make_unique<AVFrameChannels>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_channel_layout",
              std::make_unique<AVFrameChannelLayout>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_frame_best_effort_timestamp",
              std::make_unique<AVFrameChannelLayout>(Env));

  // pixfmt.h (Even AvPixFmtDesc is in this file)
  addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_nb_components",
              std::make_unique<AvPixFmtDescriptorNbComponents>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromaw",
              std::make_unique<AvPixFmtDescriptorLog2ChromaW>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromah",
              std::make_unique<AvPixFmtDescriptorLog2ChromaH>(Env));

  // samplefmt.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_packed_sample_fmt",
              std::make_unique<AVGetPackedSampleFmt>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_planar_sample_fmt",
              std::make_unique<AVGetPlanarSampleFmt>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_sample_fmt_is_planar",
              std::make_unique<AVSampleFmtIsPlanar>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_bytes_per_sample",
              std::make_unique<AVGetBytesPerSample>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_sample_fmt",
              std::make_unique<AVGetSampleFmt>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_samples_get_buffer_size",
              std::make_unique<AVSamplesGetBufferSize>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_samples_alloc_array_and_samples",
              std::make_unique<AVSamplesAllocArrayAndSamples>(Env));
  //    addHostFunc("wasmedge_ffmpeg_avutil_av_samples_copy",std::make_unique<AVSamplesCopy>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_samples_get_buffer",
              std::make_unique<AVSamplesGetBuffer>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_freep",
              std::make_unique<AVFreep>(Env));

  // channelLayout.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_channel_layout_nb_channels",
              std::make_unique<AVGetChannelLayoutNbChannels>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_get_default_channel_layout",
              std::make_unique<AVGetDefaultChannelLayout>(Env));

  // dict.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_dict_set",
              std::make_unique<AVDictSet>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_dict_get",
              std::make_unique<AVDictGet>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_dict_get_key_value",
              std::make_unique<AVDictGetKeyValue>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_dict_copy",
              std::make_unique<AVDictCopy>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_dict_free",
              std::make_unique<AVDictFree>(Env));

  // avutil_func.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_log_set_level",
              std::make_unique<AVLogSetLevel>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_log_get_level",
              std::make_unique<AVLogGetLevel>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_log_set_flags",
              std::make_unique<AVLogSetFlags>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_log_get_flags",
              std::make_unique<AVLogGetFlags>(Env));

  // time.h
  addHostFunc("wasmedge_ffmpeg_avutil_av_gettime",
              std::make_unique<AVGetTime>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_gettime_relative",
              std::make_unique<AVGetTimeRelative>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_gettime_relative_is_monotonic",
              std::make_unique<AVGetTimeRelativeIsMonotonic>(Env));
  addHostFunc("wasmedge_ffmpeg_avutil_av_usleep",
              std::make_unique<AVUSleep>(Env));
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
