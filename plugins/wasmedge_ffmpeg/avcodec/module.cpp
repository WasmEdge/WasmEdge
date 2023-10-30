#include "module.h"
#include "avCodec.h"
#include "avCodecContext.h"
#include "avCodecParameters.h"
#include "avPacket.h"
#include "avcodec_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

WasmEdgeFFmpegAVCodecModule::WasmEdgeFFmpegAVCodecModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> env)
    : ModuleInstance("wasmedge_ffmpeg_avcodec") {

  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
              std::make_unique<AVCodecAllocContext3>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
              std::make_unique<AVCodecParametersFromContext>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_free",
              std::make_unique<AVCodecParametersFree>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_free_context",
              std::make_unique<AVCodecFreeContext>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc",
              std::make_unique<AVCodecParametersAlloc>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_type",
              std::make_unique<AVCodecGetType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_open2",
              std::make_unique<AVCodecOpen2>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
              std::make_unique<AVCodecFindDecoder>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_encoder",
              std::make_unique<AVCodecIsEncoder>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_decoder",
              std::make_unique<AVCodecIsDecoder>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_close",
              std::make_unique<AVCodecClose>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",
              std::make_unique<AVCodecParametersToContext>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_receive_frame",
              std::make_unique<AVCodecReceiveFrame>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_send_packet",
              std::make_unique<AVCodecSendPacket>(env));

  // avCodecContext Struct fields access
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id",
              std::make_unique<AVCodecCtxCodecID>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type",
              std::make_unique<AVCodecCtxCodecType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_time_base",
              std::make_unique<AVCodecCtxTimeBase>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_width",
              std::make_unique<AVCodecCtxWidth>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_height",
              std::make_unique<AVCodecCtxHeight>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_aspect_ratio",
              std::make_unique<AVCodecCtxSampleAspectRatio>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_channel_layout",
              std::make_unique<AVCodecCtxChannelLayout>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_pix_fmt",
              std::make_unique<AVCodecCtxPixFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_format",
              std::make_unique<AVCodecCtxSampleFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_rate",
              std::make_unique<AVCodecCtxSampleRate>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_gop_size",
              std::make_unique<AVCodecCtxSetGopSize>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_max_b_frames",
              std::make_unique<AVCodecCtxSetMaxBFrames>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_b_quant_factor",
              std::make_unique<AVCodecCtxSetBQuantFactor>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_b_quant_offset",
              std::make_unique<AVCodecCtxSetBQuantOffset>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_i_quant_factor",
              std::make_unique<AVCodecCtxSetIQuantFactor>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_i_quant_offset",
              std::make_unique<AVCodecCtxSetIQuantOffset>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_lumi_masking",
              std::make_unique<AVCodecCtxSetLumiMasking>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_spatial_cplx_masking",
              std::make_unique<AVCodecCtxSetSpatialCplxMasking>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_p_masking",
              std::make_unique<AVCodecCtxSetPMasking>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_dark_masking",
              std::make_unique<AVCodecCtxSetDarkMasking>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_cmp",
              std::make_unique<AVCodecCtxSetMeCmp>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_sub_cmp",
              std::make_unique<AVCodecCtxSetMeSubCmp>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_cmp",
              std::make_unique<AVCodecCtxSetMbCmp>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_ildct_cmp",
              std::make_unique<AVCodecCtxSetIldctCmp>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_dia_size",
              std::make_unique<AVCodecCtxSetDiaSize>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_last_predictor_count",
              std::make_unique<AVCodecCtxSetLastPredictorsCount>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_pre_cmp",
              std::make_unique<AVCodecCtxSetMePreCmp>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_pre_dia_size",
              std::make_unique<AVCodecCtxSetPreDiaSize>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_subpel_quality",
              std::make_unique<AVCodecCtxSetMeSubpelQuality>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_range",
              std::make_unique<AVCodecCtxSetMeRange>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_decision",
              std::make_unique<AVCodecCtxSetMbDecision>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_lmin",
              std::make_unique<AVCodecCtxSetMbLMin>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_lmax",
              std::make_unique<AVCodecCtxSetMbLMax>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_intra_dc_precision",
              std::make_unique<AVCodecCtxSetIntraDcPrecision>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_qmin",
              std::make_unique<AVCodecCtxSetQMin>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_qmax",
              std::make_unique<AVCodecCtxSetQMax>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_global_quality",
              std::make_unique<AVCodecCtxSetGlobalQuality>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_colorspace",
              std::make_unique<AVCodecCtxSetColorspace>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_colorspace",
              std::make_unique<AVCodecCtxColorspace>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_color_range",
              std::make_unique<AVCodecCtxSetColorRange>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_color_range",
              std::make_unique<AVCodecCtxColorRange>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_frame_size",
              std::make_unique<AVCodecCtxFrameSize>(env));

  // avCodec Struct fields access
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_id",
              std::make_unique<AVCodecID>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_type",
              std::make_unique<AVCodecType>(env));

  // AVCodecParam Struct fields access.
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_id",
              std::make_unique<AVCodecParamCodecId>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_type",
              std::make_unique<AVCodecParamCodecType>(env));

  // AVPacket functions.
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_alloc",
              std::make_unique<AVPacketAlloc>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_new_packet",
              std::make_unique<AVNewPacket>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_unref",
              std::make_unique<AVPacketUnref>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_grow_packet",
              std::make_unique<AVGrowPacket>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_shrink_packet",
              std::make_unique<AVShrinkPacket>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_stream_index",
              std::make_unique<AVPacketStreamIndex>(env));
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
