#include "module.h"
#include "error.h"
#include "avRational.h"
#include "avFrame.h"
#include "pixfmt.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVUtil{

WasmEdgeFFmpegAVUtilModule::
    WasmEdgeFFmpegAVUtilModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avutil"){

    // error.h
    addHostFunc("wasmedge_ffmpeg_avutil_av_strerror",std::make_unique<AVUtilAVStrError>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_AVERROR",std::make_unique<AVUtilAVError>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_AVUNERROR",std::make_unique<AVUtilAVUNError>(Env));

    // rational.h
    addHostFunc("wasmedge_ffmpeg_avutil_av_add_q",std::make_unique<AVAddQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_sub_q",std::make_unique<AVSubQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_mul_q",std::make_unique<AVMulQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_div_q",std::make_unique<AVDivQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_d2q",std::make_unique<AVD2Q>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_q2d",std::make_unique<AVQ2d>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_inv_q",std::make_unique<AVInvQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_q2intfloat",std::make_unique<AVQ2IntFloat>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_nearer_q",std::make_unique<AVNearerQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_cmp_q",std::make_unique<AVCmpQ>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_reduce",std::make_unique<AVReduce>(Env));

    // frame.h
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_alloc",std::make_unique<AVFrameAlloc>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_free",std::make_unique<AVFrameFree>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_width",std::make_unique<AVFrameWidth>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_height",std::make_unique<AVFrameHeight>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_width",std::make_unique<AVFrameSetWidth>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_height",std::make_unique<AVFrameSetHeight>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_format",std::make_unique<AVFrameFormat>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_set_format",std::make_unique<AVFrameSetFormat>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_isnull",std::make_unique<AVFrameIsNull>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_linesize",std::make_unique<AVFrameLinesize>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_data",std::make_unique<AVFrameData>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_av_frame_get_buffer",std::make_unique<AVFrameGetBuffer>(Env));

    // pixfmt.h (Even AvPixFmtDesc is in this file)

    addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_nb_components",std::make_unique<AvPixFmtDescriptorNbComponents>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromaw",std::make_unique<AvPixFmtDescriptorLog2ChromaW>(Env));
    addHostFunc("wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromah",std::make_unique<AvPixFmtDescriptorLog2ChromaH>(Env));
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
