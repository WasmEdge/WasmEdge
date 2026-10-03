// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avStream.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

namespace {
AVStream **streamAt(AVFormatContext *Ctx, uint32_t Idx) {
  if (Ctx == nullptr) {
    return nullptr;
  }
  return checkedArraySlot(Ctx->streams, Ctx->nb_streams, Idx);
}
} // namespace

Expect<int32_t> AVStreamId::body(const Runtime::CallingFrame &,
                                 uint32_t AvFormatCtxId, uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamId: invalid stream index {} "
                  "(format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->id;
}

Expect<int32_t> AVStreamIndex::body(const Runtime::CallingFrame &,
                                    uint32_t AvFormatCtxId,
                                    uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamIndex: invalid stream index {} "
                  "(format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->index;
}

Expect<int32_t> AVStreamCodecPar::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx,
                                       uint32_t CodecParameterPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CodecParamId, MemInst, uint32_t, CodecParameterPtr,
                "Failed when accessing the return CodecParameter Memory"sv);

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamCodecPar: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVCodecParameters *CodecParam =
      (static_cast<AVStream *>(*AvStream))->codecpar;
  FFMPEG_PTR_STORE_CHILD(CodecParam, CodecParamId, AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamTimebase::body(const Runtime::CallingFrame &Frame,
                                       uint32_t NumPtr, uint32_t DenPtr,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr, "");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr, "");

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamTimebase: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const AvRational = static_cast<AVStream *>(*AvStream)->time_base;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamSetTimebase::body(const Runtime::CallingFrame &,
                                          uint32_t Num, uint32_t Den,
                                          uint32_t AvFormatCtxId,
                                          uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamSetTimebase: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const Timebase = av_make_q(Num, Den);
  (*AvStream)->time_base = Timebase;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVStreamDuration::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamDuration: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->duration;
}

Expect<int64_t> AVStreamStartTime::body(const Runtime::CallingFrame &,
                                        uint32_t AvFormatCtxId,
                                        uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamStartTime: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->start_time;
}

Expect<int64_t> AVStreamNbFrames::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamNbFrames: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->nb_frames;
}

Expect<int32_t> AVStreamDisposition::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId,
                                          uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamDisposition: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVStream *>(*AvStream)->disposition;
}

Expect<int32_t> AVStreamRFrameRate::body(const Runtime::CallingFrame &Frame,
                                         uint32_t NumPtr, uint32_t DenPtr,
                                         uint32_t AvFormatCtxId,
                                         uint32_t StreamIdx) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr, "");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr, "");

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamRFrameRate: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const AvRational =
      static_cast<AVStream *>(*AvStream)->r_frame_rate;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamSetRFrameRate::body(const Runtime::CallingFrame &,
                                            int32_t Num, int32_t Den,
                                            uint32_t AvFormatCtxId,
                                            uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamSetRFrameRate: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const RFrameRate = av_make_q(Num, Den);
  (*AvStream)->r_frame_rate = RFrameRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamAvgFrameRate::body(const Runtime::CallingFrame &Frame,
                                           uint32_t NumPtr, uint32_t DenPtr,
                                           uint32_t AvFormatCtxId,
                                           uint32_t StreamIdx) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr, "");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr, "");

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamAvgFrameRate: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const AvRational =
      static_cast<AVStream *>(*AvStream)->avg_frame_rate;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamSetAvgFrameRate::body(const Runtime::CallingFrame &,
                                              int32_t Num, int32_t Den,
                                              uint32_t AvFormatCtxId,
                                              uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamSetAvgFrameRate: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const AvgFrameRate = av_make_q(Num, Den);
  (*AvStream)->avg_frame_rate = AvgFrameRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamMetadata::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx, uint32_t DictPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed when accessing the return AVDictPtr Memory"sv);

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamMetadata: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  FFMPEG_PTR_STORE_CHILD(&(*AvStream)->metadata, DictId, AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamSetMetadata::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId,
                                          uint32_t StreamIdx, uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);

  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamSetMetadata: invalid stream "
                  "index {} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  FFMPEG_PTR_CHECK_NONZERO(AvDictionary, DictId,
                           static_cast<int32_t>(ErrNo::InternalError));
  return applyMetadataCopy(&(*AvStream)->metadata, AvDictionary);
}

Expect<int32_t> AVStreamDiscard::body(const Runtime::CallingFrame &,
                                      uint32_t AvFormatCtxId,
                                      uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = streamAt(AvFormatContext, StreamIdx);
  if (AvStream == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVStreamDiscard: invalid stream index "
                  "{} (format context id {}, nb_streams={})"sv,
                  StreamIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_streams : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<int32_t>((*AvStream)->discard);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
