// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avformatContext.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVFormatCtxIFormat::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvFormatCtxId,
                                         uint32_t AvInputFormatPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvInputFormatId, MemInst, uint32_t, AvInputFormatPtr,
                "Failed when accessing the return AVInputFormat Memory"sv);

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  AVInputFormat const *AvInputFormat = AvFormatCtx->iformat;
  FFMPEG_PTR_STORE(const_cast<AVInputFormat *>(AvInputFormat), AvInputFormatId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatCtxOFormat::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvFormatCtxId,
                                         uint32_t AvOutputFormatPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvOutputFormatId, MemInst, uint32_t, AvOutputFormatPtr,
                "Failed when accessing the return AVOutputFormat Memory"sv);

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  AVOutputFormat const *AvOutputFormat = AvFormatCtx->oformat;
  FFMPEG_PTR_STORE(const_cast<AVOutputFormat *>(AvOutputFormat),
                   AvOutputFormatId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatCtxProbeScore::body(const Runtime::CallingFrame &,
                                            uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->probe_score;
}

Expect<uint32_t> AVFormatCtxNbStreams::body(const Runtime::CallingFrame &,
                                            uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->nb_streams;
};

Expect<int64_t> AVFormatCtxBitRate::body(const Runtime::CallingFrame &,
                                         uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->bit_rate;
}

Expect<int64_t> AVFormatCtxDuration::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->duration;
}

Expect<uint32_t> AVFormatCtxNbChapters::body(const Runtime::CallingFrame &,
                                             uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->nb_chapters;
}

Expect<int32_t> AVFormatCtxSetNbChapters::body(const Runtime::CallingFrame &,
                                               uint32_t AvFormatCtxId,
                                               uint32_t NbChapters) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AvFormatContext->nb_chapters = NbChapters;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatCtxMetadata::body(const Runtime::CallingFrame &Frame,
                                          uint32_t AvFormatCtxId,
                                          uint32_t DictPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed when accessing the return AVDictionary memory"sv);

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  AVDictionary **AvDictionary =
      static_cast<AVDictionary **>(av_malloc(sizeof(AVDictionary *)));

  *AvDictionary = AvFormatCtx->metadata;
  FFMPEG_PTR_STORE(AvDictionary, DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatCtxSetMetadata::body(const Runtime::CallingFrame &,
                                             uint32_t AvFormatCtxId,
                                             uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);

  if (AvDictionary == nullptr) {
    AvFormatCtx->metadata = nullptr;
  } else {
    AvFormatCtx->metadata = *AvDictionary;
  }
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
