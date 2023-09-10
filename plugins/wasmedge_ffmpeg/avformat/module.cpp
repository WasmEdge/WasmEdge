#include "module.h"
#include "avformat_func.h"
#include "avformatContext.h"
#include "avio_func.h"
#include "avInputFormat.h"
#include "avStream.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVFormat{

WasmEdgeFFmpegAVFormatModule::
WasmEdgeFFmpegAVFormatModule(std::shared_ptr<WasmEdgeFFmpegEnv> env )
    : ModuleInstance("wasmedge_ffmpeg_avformat") {

  // Module name + Func name
  // avformat.h
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_open_input",std::make_unique<AVFormatOpenInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_find_stream_info",std::make_unique<AVFormatFindStreamInfo>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_close_input",std::make_unique<AVFormatCloseInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_play",std::make_unique<AVReadPlay>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_pause",std::make_unique<AVReadPause>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_dump_format",std::make_unique<AVDumpFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_seek_file",std::make_unique<AVFormatSeekFile>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_free_context",std::make_unique<AVFormatFreeContext>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_find_best_stream",std::make_unique<AVFindBestStream>(env));

  //avio.h
  addHostFunc("wasmedge_ffmpeg_avformat_avio_close",std::make_unique<AVIOClose>(env));

  // avformatContext Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_iformat",std::make_unique<AVFormatCtxIFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_probescope",std::make_unique<AVFormatCtxProbeScore>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_streams",std::make_unique<AVFormatCtxNbStreams>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_duration",std::make_unique<AVFormatCtxDuration>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_bit_rate",std::make_unique<AVFormatCtxBitRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_chapters",std::make_unique<AVFormatCtxNbChapters>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_avstream",std::make_unique<AVFormatCtxGetAVStream>(env));

  // avInputFormat Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_name",std::make_unique<AVInputFormat_name>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_long_name",std::make_unique<AVInputFormat_long_name>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_extensions",std::make_unique<AVInputFormat_extensions>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_mime_type",std::make_unique<AVInputFormat_mime_type>(env));

  // avStream
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_id",std::make_unique<AVStreamId>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_index",std::make_unique<AVStreamIndex>(env));

}


} // AVFormat
} // WasmEdgeFFmpeg
} // Host
} // WasmEdge
