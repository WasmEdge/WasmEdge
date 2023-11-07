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
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
              std::make_unique<AVCodecFindEncoder>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_receive_packet",
              std::make_unique<AVCodecReceivePacket>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_send_frame",
              std::make_unique<AVCodecSendFrame>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name",
              std::make_unique<AVCodecFindDecoderByName>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name",
              std::make_unique<AVCodecFindEncoderByName>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_rescale_ts",
              std::make_unique<AVPacketRescaleTs>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_ref",
              std::make_unique<AVPacketRef>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_make_writable",
              std::make_unique<AVPacketMakeWritable>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_copy",
              std::make_unique<AVCodecParametersCopy>(env));

  // avCodecContext Struct fields access
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id",
              std::make_unique<AVCodecCtxCodecID>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type",
              std::make_unique<AVCodecCtxCodecType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_codec_type",
              std::make_unique<AVCodecCtxSetCodecType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_time_base",
              std::make_unique<AVCodecCtxTimeBase>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_width",
              std::make_unique<AVCodecCtxWidth>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_width",
              std::make_unique<AVCodecCtxSetWidth>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_height",
              std::make_unique<AVCodecCtxHeight>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_height",
              std::make_unique<AVCodecCtxSetHeight>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_aspect_ratio",
              std::make_unique<AVCodecCtxSampleAspectRatio>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_channel_layout",
              std::make_unique<AVCodecCtxChannelLayout>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_pix_fmt",
              std::make_unique<AVCodecCtxPixFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_format",
              std::make_unique<AVCodecCtxSampleFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
              std::make_unique<AVCodecCtxSetSampleFormat>(env));
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
  addHostFunc(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_temporal_cplx_masking",
      std::make_unique<AVCodecCtxSetSpatialCplxMasking>(env));
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
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_bit_rate",
              std::make_unique<AVCodecCtxSetBitRate>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_rc_max_rate",
              std::make_unique<AVCodecCtxSetRcMaxRate>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_bit_rate_tolerance",
              std::make_unique<AVCodecCtxSetBitRateTolerance>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_compression_level",
              std::make_unique<AVCodecCtxSetCompressionLevel>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_time_base",
              std::make_unique<AVCodecCtxSetTimebase>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_set_framerate",
              std::make_unique<AVCodecCtxSetFrameRate>(env));

  // avCodec Struct fields access
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_id",
              std::make_unique<AVCodecID>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_type",
              std::make_unique<AVCodecType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_max_lowres",
              std::make_unique<AVCodecMaxLowres>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_capabilities",
              std::make_unique<AVCodecCapabilities>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_name_len",
              std::make_unique<AVCodecGetNameLen>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_name",
              std::make_unique<AVCodecGetName>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_long_name_len",
              std::make_unique<AVCodecGetLongNameLen>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_long_name",
              std::make_unique<AVCodecGetLongName>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_profiles",
              std::make_unique<AVCodecProfiles>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_is_null",
              std::make_unique<AVCodecPixFmtsIsNull>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_iter",
              std::make_unique<AVCodecPixFmtsIter>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_is_null",
              std::make_unique<AVCodecSupportedFrameratesIsNull>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_iter",
              std::make_unique<AVCodecSupportedFrameratesIter>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_is_null",
              std::make_unique<AVCodecSupportedSampleRatesIsNull>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_iter",
              std::make_unique<AVCodecSupportedSampleRatesIter>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_is_null",
              std::make_unique<AVCodecChannelLayoutIsNull>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_iter",
              std::make_unique<AVCodecChannelLayoutIter>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_is_null",
              std::make_unique<AVCodecSampleFmtsIsNull>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_iter",
              std::make_unique<AVCodecSampleFmtsIter>(env));

  // AVCodecParam Struct fields access.
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_id",
              std::make_unique<AVCodecParamCodecId>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_type",
              std::make_unique<AVCodecParamCodecType>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag",
              std::make_unique<AVCodecParamSetCodecTag>(env));

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
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_stream_index",
              std::make_unique<AVPacketSetStreamIndex>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_size",
              std::make_unique<AVPacketSize>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_flags",
              std::make_unique<AVPacketFlags>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_flags",
              std::make_unique<AVPacketSetFlags>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_pos",
              std::make_unique<AVPacketPos>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_pos",
              std::make_unique<AVPacketSetPos>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_duration",
              std::make_unique<AVPacketDuration>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_duration",
              std::make_unique<AVPacketSetDuration>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_dts",
              std::make_unique<AVPacketDts>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_dts",
              std::make_unique<AVPacketSetDts>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_pts",
              std::make_unique<AVPacketPts>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_set_pts",
              std::make_unique<AVPacketSetPts>(env));
  addHostFunc("wasmedge_ffmpeg_avcodec_av_packet_is_data_null",
              std::make_unique<AVPacketIsDataNull>(env));
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
