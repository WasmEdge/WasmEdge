// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "avFilter.h"
#include "avfilter_func.h"
#include "buffer_source_sink.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

WasmEdgeFFmpegAVFilterModule::WasmEdgeFFmpegAVFilterModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avfilter") {
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_alloc",
              std::make_unique<AVFilterGraphAlloc>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_config",
              std::make_unique<AVFilterGraphConfig>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_free",
              std::make_unique<AVFilterGraphFree>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_get_filter",
              std::make_unique<AVFilterGraphGetFilter>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_parse_ptr",
              std::make_unique<AVFilterGraphParsePtr>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_free",
              std::make_unique<AVFilterInOutFree>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_version",
              std::make_unique<AVFilterVersion>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_get_by_name",
              std::make_unique<AVFilterGetByName>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_configuration_length",
              std::make_unique<AVFilterConfigurationLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_configuration",
              std::make_unique<AVFilterConfiguration>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_license_length",
              std::make_unique<AVFilterLicenseLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_license",
              std::make_unique<AVFilterLicense>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_create_filter",
              std::make_unique<AVFilterGraphCreateFilter>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_alloc",
              std::make_unique<AVFilterInOutAlloc>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_pad_get_name_length",
              std::make_unique<AVFilterPadGetNameLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_pad_get_name",
              std::make_unique<AVFilterPadGetName>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_pad_get_type",
              std::make_unique<AVFilterPadGetType>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_dump_length",
              std::make_unique<AVFilterGraphDumpLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_graph_dump",
              std::make_unique<AVFilterGraphDump>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_free_graph_str",
              std::make_unique<AVFilterFreeGraphStr>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_drop",
              std::make_unique<AVFilterDrop>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_pad_drop",
              std::make_unique<AVFilterPadDrop>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_context_drop",
              std::make_unique<AVFilterContextDrop>(Env));

  // buffersrc.h && buffersink.h
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_get_frame",
              std::make_unique<AVBufferSinkGetFrame>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_get_samples",
              std::make_unique<AVBufferSinkGetSamples>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersink_set_frame_size",
              std::make_unique<AvBufferSinkSetFrameSize>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_get_nb_failed_requests",
              std::make_unique<AVBufferSrcGetNbFailedRequests>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_add_frame",
              std::make_unique<AVBufferSrcAddFrame>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_av_buffersrc_close",
              std::make_unique<AVBufferSrcClose>(Env));

  // avfilter.h
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_name_length",
              std::make_unique<AVFilterNameLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_name",
              std::make_unique<AVFilterName>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_description_length",
              std::make_unique<AVFilterDescriptionLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_description",
              std::make_unique<AVFilterDescription>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_nb_inputs",
              std::make_unique<AVFilterNbInputs>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_nb_outputs",
              std::make_unique<AVFilterNbOutputs>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_flags",
              std::make_unique<AVFilterFlags>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_set_name",
              std::make_unique<AVFilterInOutSetName>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_set_filter_ctx",
              std::make_unique<AVFilterInOutSetFilterCtx>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_set_pad_idx",
              std::make_unique<AVFilterInOutSetPadIdx>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_inout_set_next",
              std::make_unique<AVFilterInOutSetNext>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_get_inputs_filter_pad",
              std::make_unique<AVFilterGetInputsFilterPad>(Env));
  addHostFunc("wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad",
              std::make_unique<AVFilterGetOutputsFilterPad>(Env));
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
