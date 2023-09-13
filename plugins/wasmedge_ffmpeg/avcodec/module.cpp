#include "module.h"
#include "avcodec_func.h"
#include "avCodecContext.h"
#include "avCodec.h"
#include "avCodecParameters.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVcodec {

WasmEdgeFFmpegAVCodecModule::
WasmEdgeFFmpegAVCodecModule(std::shared_ptr<WasmEdgeFFmpegEnv> env )
    : ModuleInstance("wasmedge_ffmpeg_avcodec") {

    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",std::make_unique<AVCodecAllocContext3>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",std::make_unique<AVCodecParametersFromContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",std::make_unique<AVCodecParametersToContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_free",std::make_unique<AVCodecParametersFree>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_free_context",std::make_unique<AVCodecFreeContext>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc",std::make_unique<AVCodecParametersAlloc>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_get_type",std::make_unique<AVCodecGetType>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_open2",std::make_unique<AVCodecOpen2>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_open2",std::make_unique<AVCodecOpen2>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",std::make_unique<AVCodecFindDecoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_encoder",std::make_unique<AVCodecIsEncoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_av_codec_is_decoder",std::make_unique<AVCodecIsDecoder>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_close",std::make_unique<AVCodecClose>(env));

    //avCodecContext Struct fields access
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id",std::make_unique<AVCodecCtxCodecID>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type",std::make_unique<AVCodecCtxCodecType>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodeccontext_time_base",std::make_unique<AVCodecCtxTimeBase>(env));

    //avCodec Struct fields access
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_id",std::make_unique<AVCodec_ID>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodec_type",std::make_unique<AVCodec_Type>(env));

    // AVCodecParam Struct fields access.
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_id",std::make_unique<AVCodecParam_codecId>(env));
    addHostFunc("wasmedge_ffmpeg_avcodec_avcodecparam_codec_type",std::make_unique<AVCodecParam_codecType>(env));
}

}
}
}
}
