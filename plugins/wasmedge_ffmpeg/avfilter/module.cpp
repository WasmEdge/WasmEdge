#include "module.h"
#include "avfilter_func.h"
#include "buffer_source_sink.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter{

WasmEdgeFFmpegAVFilterModule::
WasmEdgeFFmpegAVFilterModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avfilter"){

    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_alloc",std::make_unique<AVFilterGraphAlloc>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_config",std::make_unique<AVFilterGraphConfig>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_free",std::make_unique<AVFilterGraphFree>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_get_filter",std::make_unique<AVFilterGraphGetFilter>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_parse_ptr",std::make_unique<AVFilterGraphParsePtr>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_free",std::make_unique<AVFilterInOutFree>(Env));


    // buffersrc.h && buffersink.h
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_get_frame",std::make_unique<AVBufferSinkGetFrame>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_get_samples",std::make_unique<AVBufferSinkGetSamples>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_set_frame_size",std::make_unique<AvBufferSinkSetFrameSize>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_get_nb_failed_requests",std::make_unique<AVBufferSrcGetNbFailedRequests>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_add_frame",std::make_unique<AVBufferSrcAddFrame>(Env));
    addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_close",std::make_unique<AVBufferSrcClose>(Env));
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
