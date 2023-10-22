#include "module.h"
#include "avChapter.h"
#include "avInputOutputFormat.h"
#include "avStream.h"
#include "avformatContext.h"
#include "avformat_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

WasmEdgeFFmpegAVFormatModule::WasmEdgeFFmpegAVFormatModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> env)
    : ModuleInstance("wasmedge_ffmpeg_avformat") {

  // Module name + Func name
  // avformat.h
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_open_input",
              std::make_unique<AVFormatOpenInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_find_stream_info",
              std::make_unique<AVFormatFindStreamInfo>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_close_input",
              std::make_unique<AVFormatCloseInput>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_play",
              std::make_unique<AVReadPlay>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_pause",
              std::make_unique<AVReadPause>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_dump_format",
              std::make_unique<AVDumpFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_seek_file",
              std::make_unique<AVFormatSeekFile>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_free_context",
              std::make_unique<AVFormatFreeContext>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_find_best_stream",
              std::make_unique<AVFindBestStream>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_frame",
              std::make_unique<AVReadFrame>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_close",
              std::make_unique<AVIOClose>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_network_init",
              std::make_unique<AVFormatNetworkInit>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_network_deinit",
              std::make_unique<AVFormatNetworkDeInit>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_write_header", // need to test
              std::make_unique<AVFormatWriteHeader>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_write_trailer", // need to test
              std::make_unique<AVFormatWriteTrailer>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
              std::make_unique<AVFormatAllocOutputContext2>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_open",
              std::make_unique<AVIOOpen>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_open2",
              std::make_unique<AVIOOpen2>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avchapter_mallocz",
              std::make_unique<AVChapterMallocz>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avchapter_dynarray_add", // need to test
              std::make_unique<AVChapterDynarrayAdd>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_avfreep",
              std::make_unique<AVFreeP>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_version",
              std::make_unique<AVFormatVersion>(env));

  // avformatContext Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_iformat",
              std::make_unique<AVFormatCtxIFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_oformat",
              std::make_unique<AVFormatCtxOFormat>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_probescope",
              std::make_unique<AVFormatCtxProbeScore>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_streams",
              std::make_unique<AVFormatCtxNbStreams>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_duration",
              std::make_unique<AVFormatCtxDuration>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_bit_rate",
              std::make_unique<AVFormatCtxBitRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_chapters",
              std::make_unique<AVFormatCtxNbChapters>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_set_nb_chapters",
              std::make_unique<AVFormatCtxSetNbChapters>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_metadata",
              std::make_unique<AVFormatCtxMetadata>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_set_metadata",
              std::make_unique<AVFormatCtxSetMetadata>(env));

  // avInputFormat Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_name_length",
              std::make_unique<AVIOFormatNameLength>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_name",
              std::make_unique<AVInputFormatName>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_name",
              std::make_unique<AVOutputFormatName>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_long_name_length",
              std::make_unique<AVIOFormatLongNameLength>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_long_name",
              std::make_unique<AVInputFormatLongName>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_long_name",
              std::make_unique<AVOutputFormatExtensions>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_extensions_length",
              std::make_unique<AVIOFormatExtensionsLength>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_extensions",
              std::make_unique<AVInputFormatExtensions>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_extensions",
              std::make_unique<AVOutputFormatExtensions>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_mime_type_length",
              std::make_unique<AVIOFormatMimeTypeLength>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_mime_type",
              std::make_unique<AVInputFormatMimeType>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_mime_type",
              std::make_unique<AVOutputFormatMimeType>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_flags",
              std::make_unique<AVOutputFormatMimeType>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputOutputFormat_free",
              std::make_unique<AVInputOutputFormatFree>(env));
  //  addHostFunc("wasmedge_ffmpeg_avformat_av_guess_codec",
  //              std::make_unique<AVGuessCodec>(env));

  // avStream Struct Functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_id",
              std::make_unique<AVStreamId>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_index",
              std::make_unique<AVStreamIndex>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_codecpar",
              std::make_unique<AVStreamCodecPar>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_timebase",
              std::make_unique<AVStreamTimebase>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_timebase",
              std::make_unique<AVStreamSetTimebase>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_duration",
              std::make_unique<AVStreamDuration>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_start_time",
              std::make_unique<AVStreamStartTime>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_nb_frames",
              std::make_unique<AVStreamNbFrames>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_disposition",
              std::make_unique<AVStreamDisposition>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_r_frame_rate",
              std::make_unique<AVStreamRFrameRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_r_frame_rate",
              std::make_unique<AVStreamSetRFrameRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_avg_frame_rate",
              std::make_unique<AVStreamAvgFrameRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_avg_frame_rate",
              std::make_unique<AVStreamSetAvgFrameRate>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_metadata",
              std::make_unique<AVStreamMetadata>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_metadata",
              std::make_unique<AVStreamSetMetadata>(env));

  // avChapter Struct Functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_id",
              std::make_unique<AVChapterId>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_id",
              std::make_unique<AVChapterSetId>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_timebase",
              std::make_unique<AVChapterTimebase>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_timebase",
              std::make_unique<AVChapterSetTimebase>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_start",
              std::make_unique<AVChapterStart>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_start",
              std::make_unique<AVChapterSetStart>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_end",
              std::make_unique<AVChapterEnd>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_end",
              std::make_unique<AVChapterSetEnd>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_metadata",
              std::make_unique<AVChapterMetadata>(env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_metadata",
              std::make_unique<AVChapterSetMetadata>(env));
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
