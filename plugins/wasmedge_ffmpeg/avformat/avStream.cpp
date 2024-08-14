// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avStream.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVStreamId::body(const Runtime::CallingFrame &,
                                 uint32_t AvFormatCtxId, uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  return static_cast<AVStream *>(*AvStream)->id;
}

Expect<int32_t> AVStreamIndex::body(const Runtime::CallingFrame &,
                                    uint32_t AvFormatCtxId,
                                    uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  AVCodecParameters *CodecParam =
      (static_cast<AVStream *>(*AvStream))->codecpar;
  FFMPEG_PTR_STORE(CodecParam, CodecParamId);
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
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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

  AVStream **AvStream = AvFormatContext->streams;
  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  AVRational const Timebase = av_make_q(Num, Den);
  (*AvStream)->time_base = Timebase;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVStreamDuration::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  return static_cast<AVStream *>(*AvStream)->duration;
}

Expect<int64_t> AVStreamStartTime::body(const Runtime::CallingFrame &,
                                        uint32_t AvFormatCtxId,
                                        uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  return static_cast<AVStream *>(*AvStream)->start_time;
}

Expect<int64_t> AVStreamNbFrames::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  return static_cast<AVStream *>(*AvStream)->nb_frames;
}

Expect<int32_t> AVStreamDisposition::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId,
                                          uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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

  AVStream **AvStream = AvFormatContext->streams;
  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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

  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
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

  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  AVDictionary **AvDictionary =
      static_cast<AVDictionary **>(av_malloc(sizeof(AVDictionary *)));

  *AvDictionary = (*AvStream)->metadata;
  FFMPEG_PTR_STORE(AvDictionary, DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamSetMetadata::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId,
                                          uint32_t StreamIdx, uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);

  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  if (AvDictionary == nullptr) {
    (*AvStream)->metadata = nullptr;
  } else {
    (*AvStream)->metadata = *AvDictionary;
  }

  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVStreamDiscard::body(const Runtime::CallingFrame &,
                                      uint32_t AvFormatCtxId,
                                      uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  for (unsigned int I = 1; I <= StreamIdx; I++) {
    AvStream++;
  }

  return static_cast<int32_t>((*AvStream)->discard);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
