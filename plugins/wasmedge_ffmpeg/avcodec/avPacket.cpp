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
                "Failed when accessing the return AVCodecContext Memory");

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

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
