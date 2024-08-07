// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avformat") {
  // avformat_func.h
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_open_input",
              std::make_unique<AVFormatOpenInput>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_find_stream_info",
              std::make_unique<AVFormatFindStreamInfo>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_close_input",
              std::make_unique<AVFormatCloseInput>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_play",
              std::make_unique<AVReadPlay>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_pause",
              std::make_unique<AVReadPause>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_dump_format",
              std::make_unique<AVDumpFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_seek_file",
              std::make_unique<AVFormatSeekFile>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_free_context",
              std::make_unique<AVFormatFreeContext>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_find_best_stream",
              std::make_unique<AVFindBestStream>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_read_frame", // TODO: Write Test
              std::make_unique<AVReadFrame>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_close",
              std::make_unique<AVIOClose>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_network_init",
              std::make_unique<AVFormatNetworkInit>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_network_deinit",
              std::make_unique<AVFormatNetworkDeInit>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_write_header",
              std::make_unique<AVFormatWriteHeader>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_write_trailer",
              std::make_unique<AVFormatWriteTrailer>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
              std::make_unique<AVFormatAllocOutputContext2>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_open",
              std::make_unique<AVIOOpen>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avio_open2",
              std::make_unique<AVIOOpen2>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avchapter_mallocz",
              std::make_unique<AVChapterMallocz>(Env));
  addHostFunc(
      "wasmedge_ffmpeg_avformat_avchapter_dynarray_add", // TODO: Write Test
      std::make_unique<AVChapterDynarrayAdd>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_avfreep",
              std::make_unique<AVFreeP>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_version",
              std::make_unique<AVFormatVersion>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_write_frame",
              std::make_unique<AVWriteFrame>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_interleaved_write_frame",
              std::make_unique<AVInterleavedWriteFrame>(Env));
  addHostFunc(
      "wasmedge_ffmpeg_avformat_avformat_new_stream", // TODO: Write Test
      std::make_unique<AVFormatNewStream>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_av_guess_codec", // TODO: Write Test
              std::make_unique<AVGuessCodec>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_configuration_length",
              std::make_unique<AVFormatConfigurationLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_configuration",
              std::make_unique<AVFormatConfiguration>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_license_length",
              std::make_unique<AVFormatLicenseLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformat_license",
              std::make_unique<AVFormatLicense>(Env));

  // avformatContext Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_iformat",
              std::make_unique<AVFormatCtxIFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_oformat",
              std::make_unique<AVFormatCtxOFormat>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_probescope",
              std::make_unique<AVFormatCtxProbeScore>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_streams",
              std::make_unique<AVFormatCtxNbStreams>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_duration",
              std::make_unique<AVFormatCtxDuration>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_bit_rate",
              std::make_unique<AVFormatCtxBitRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_nb_chapters",
              std::make_unique<AVFormatCtxNbChapters>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_set_nb_chapters",
              std::make_unique<AVFormatCtxSetNbChapters>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_metadata",
              std::make_unique<AVFormatCtxMetadata>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avformatContext_set_metadata",
              std::make_unique<AVFormatCtxSetMetadata>(Env));

  // avInputFormat Struct functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_name_length",
              std::make_unique<AVIOFormatNameLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_name",
              std::make_unique<AVInputFormatName>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_name",
              std::make_unique<AVOutputFormatName>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_long_name_length",
              std::make_unique<AVIOFormatLongNameLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_long_name",
              std::make_unique<AVInputFormatLongName>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_long_name",
              std::make_unique<AVOutputFormatExtensions>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_extensions_length",
              std::make_unique<AVIOFormatExtensionsLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_extensions",
              std::make_unique<AVInputFormatExtensions>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_extensions",
              std::make_unique<AVOutputFormatExtensions>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avIOFormat_mime_type_length",
              std::make_unique<AVIOFormatMimeTypeLength>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputFormat_mime_type",
              std::make_unique<AVInputFormatMimeType>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_mime_type",
              std::make_unique<AVOutputFormatMimeType>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avOutputFormat_flags",
              std::make_unique<AVOutputFormatFlags>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avInputOutputFormat_free",
              std::make_unique<AVInputOutputFormatFree>(Env));

  // avStream Struct Functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_id",
              std::make_unique<AVStreamId>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_index",
              std::make_unique<AVStreamIndex>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_codecpar",
              std::make_unique<AVStreamCodecPar>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_timebase",
              std::make_unique<AVStreamTimebase>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_timebase",
              std::make_unique<AVStreamSetTimebase>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_duration",
              std::make_unique<AVStreamDuration>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_start_time",
              std::make_unique<AVStreamStartTime>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_nb_frames",
              std::make_unique<AVStreamNbFrames>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_disposition",
              std::make_unique<AVStreamDisposition>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_r_frame_rate",
              std::make_unique<AVStreamRFrameRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_r_frame_rate",
              std::make_unique<AVStreamSetRFrameRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_avg_frame_rate",
              std::make_unique<AVStreamAvgFrameRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_avg_frame_rate",
              std::make_unique<AVStreamSetAvgFrameRate>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_metadata",
              std::make_unique<AVStreamMetadata>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_set_metadata",
              std::make_unique<AVStreamSetMetadata>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avStream_discard",
              std::make_unique<AVStreamDiscard>(Env));

  // avChapter Struct Functions.
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_id",
              std::make_unique<AVChapterId>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_id",
              std::make_unique<AVChapterSetId>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_timebase",
              std::make_unique<AVChapterTimebase>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_timebase",
              std::make_unique<AVChapterSetTimebase>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_start",
              std::make_unique<AVChapterStart>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_start",
              std::make_unique<AVChapterSetStart>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_end",
              std::make_unique<AVChapterEnd>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_end",
              std::make_unique<AVChapterSetEnd>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_metadata",
              std::make_unique<AVChapterMetadata>(Env));
  addHostFunc("wasmedge_ffmpeg_avformat_avChapter_set_metadata",
              std::make_unique<AVChapterSetMetadata>(Env));
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
