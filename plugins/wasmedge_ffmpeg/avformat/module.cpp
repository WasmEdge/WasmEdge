#include "module.h"
#include "avformat_func.h"
#include "avformatContext.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVFormat{

WasmEdgeFFmpegAVFormatModule::
WasmEdgeFFmpegAVFormatModule(std::shared_ptr<WasmEdgeFFmpegEnv> env )
    : ModuleInstance("wasmedge_ffmpeg_avformat") {

  // Module name + Func name
  // avformat.h
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_open_input",
              std::make_unique<AVFormatOpenInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_find_stream_info",
              std::make_unique<AVFormatFindStreamInfo>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_close_input",std::make_unique<AVFormatCloseInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_play",std::make_unique<AVReadPlay>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_pause",std::make_unique<AVReadPause>(env));

  // avformatContext Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avformatCtxIFormat",std::make_unique<AVFormatCtxIFormat>(env));
}


} // AVFormat
} // WasmEdgeFFmpeg
} // Host
} // WasmEdge
