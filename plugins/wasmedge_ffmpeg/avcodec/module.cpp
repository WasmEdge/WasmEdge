#include "module.h"
#include "avcodec_func.h"
#include "avCodecContext.h"
#include "avCodec.h"
#include "avCodecParameters.h"
#include "avPacket.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVcodec {

WasmEdgeFFmpegAVCodecModule::
WasmEdgeFFmpegAVCodecModule(std::shared_ptr<WasmEdgeFFmpegEnv> env )
    : ModuleInstance("wasmedge_ffmpeg_avcodec") {

    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",std::make_unique<AVCodecAllocContext3>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",std::make_unique<AVCodecParametersFromContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_free",std::make_unique<AVCodecParametersFree>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_free_context",std::make_unique<AVCodecFreeContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc",std::make_unique<AVCodecParametersAlloc>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_type",std::make_unique<AVCodecGetType>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_open2",std::make_unique<AVCodecOpen2>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",std::make_unique<AVCodecFindDecoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_encoder",std::make_unique<AVCodecIsEncoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_decoder",std::make_unique<AVCodecIsDecoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_close",std::make_unique<AVCodecClose>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",std::make_unique<AVCodecParametersToContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_receive_frame",std::make_unique<AVCodecReceiveFrame>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_send_packet",std::make_unique<AVCodecSendPacket>(env));

    //avCodecContext Struct fields access
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id",std::make_unique<AVCodecCtxCodecID>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type",std::make_unique<AVCodecCtxCodecType>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_time_base",std::make_unique<AVCodecCtxTimeBase>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_width",std::make_unique<AVCodecCtxWidth>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_height",std::make_unique<AVCodecCtxHeight>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_aspect_ratio",std::make_unique<AVCodecCtxSampleAspectRatio>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_channel_layout",std::make_unique<AVCodecCtxChannelLayout>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_pix_fmt",std::make_unique<AVCodecCtxPixFormat>(env));

    //avCodec Struct fields access
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_id",std::make_unique<AVCodecID>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_type",std::make_unique<AVCodecType>(env));

    // AVCodecParam Struct fields access.
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_id",std::make_unique<AVCodecParamCodecId>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_type",std::make_unique<AVCodecParamCodecType>(env));

    // AVPacket functions.
    addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_alloc",std::make_unique<AVPacketAlloc>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_new_packet",std::make_unique<AVNewPacket>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_unref",std::make_unique<AVPacketUnref>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_grow_packet",std::make_unique<AVGrowPacket>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_shrink_packet",std::make_unique<AVShrinkPacket>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_stream_index",std::make_unique<AVPacketStreamIndex>(env));
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
