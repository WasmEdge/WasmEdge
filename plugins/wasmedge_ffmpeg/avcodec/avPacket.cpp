// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avPacket.h"

extern "C" {
#include "libavcodec/packet.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<int32_t> AVPacketAlloc::body(const Runtime::CallingFrame &Frame,
                                    uint32_t AvPacketPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvPacketId, MemInst, uint32_t, AvPacketPtr,
                "Failed when accessing the return AVCodecContext Memory"sv);

  FFMPEG_PTR_FETCH(AvPacket, *AvPacketId, AVPacket); // Initialize the packet.
  AvPacket = av_packet_alloc();
  FFMPEG_PTR_STORE(AvPacket, AvPacketId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVNewPacket::body(const Runtime::CallingFrame &,
                                  uint32_t AvPacketId, int32_t Size) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return av_new_packet(AvPacket, Size);
}

Expect<int32_t> AVPacketRef::body(const Runtime::CallingFrame &,
                                  uint32_t DestPacketId, uint32_t SrcPacketId) {
  FFMPEG_PTR_FETCH(DestAvPacket, DestPacketId, AVPacket);
  FFMPEG_PTR_FETCH(SrcAvPacket, SrcPacketId, AVPacket);

  return av_packet_ref(DestAvPacket, SrcAvPacket);
}

Expect<int32_t> AVPacketUnref::body(const Runtime::CallingFrame &,
                                    uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket); // Free packet.
  av_packet_unref(AvPacket);
  FFMPEG_PTR_DELETE(AvPacketId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVGrowPacket::body(const Runtime::CallingFrame &,
                                   uint32_t AvPacketId, int32_t Size) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return av_grow_packet(AvPacket, Size);
}

Expect<int32_t> AVShrinkPacket::body(const Runtime::CallingFrame &,
                                     uint32_t AvPacketId, int32_t Size) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  av_shrink_packet(AvPacket, Size);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPacketStreamIndex::body(const Runtime::CallingFrame &,
                                          uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->stream_index;
}

Expect<int32_t> AVPacketSetStreamIndex::body(const Runtime::CallingFrame &,
                                             uint32_t AvPacketId,
                                             int32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->stream_index = StreamIdx;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPacketSize::body(const Runtime::CallingFrame &,
                                   uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->size;
}

Expect<int32_t> AVPacketFlags::body(const Runtime::CallingFrame &,
                                    uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->flags;
}

Expect<int32_t> AVPacketSetFlags::body(const Runtime::CallingFrame &,
                                       uint32_t AvPacketId, int32_t Flags) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->flags = Flags;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVPacketPos::body(const Runtime::CallingFrame &,
                                  uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->pos;
}

Expect<int32_t> AVPacketSetPos::body(const Runtime::CallingFrame &,
                                     uint32_t AvPacketId, int64_t Pos) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->pos = Pos;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVPacketDuration::body(const Runtime::CallingFrame &,
                                       uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->duration;
}

Expect<int32_t> AVPacketSetDuration::body(const Runtime::CallingFrame &,
                                          uint32_t AvPacketId,
                                          int64_t Duration) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->duration = Duration;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVPacketDts::body(const Runtime::CallingFrame &,
                                  uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->dts;
}

Expect<int32_t> AVPacketSetDts::body(const Runtime::CallingFrame &,
                                     uint32_t AvPacketId, int64_t Dts) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->dts = Dts;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVPacketPts::body(const Runtime::CallingFrame &,
                                  uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  return AvPacket->pts;
}

Expect<int32_t> AVPacketSetPts::body(const Runtime::CallingFrame &,
                                     uint32_t AvPacketId, int64_t Pts) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AvPacket->pts = Pts;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPacketIsDataNull::body(const Runtime::CallingFrame &,
                                         uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  if (AvPacket->data == nullptr)
    return 1;
  return 0;
}

Expect<int32_t> AVPacketData::body(const Runtime::CallingFrame &Frame,
                                   uint32_t AvPacketId, uint32_t DataPtr,
                                   uint32_t DataLen) {
  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_SPAN_CHECK(Buffer, MemInst, uint8_t, DataPtr, DataLen, "");

  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  uint8_t *Data = AvPacket->data;
  std::copy_n(Data, DataLen, Buffer.data());
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
