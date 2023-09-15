#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVPacketAlloc : public WasmEdgeFFmpegAVCodec<AVPacketAlloc> {
public:
  AVPacketAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketPtr);
};

class AVNewPacket : public WasmEdgeFFmpegAVCodec<AVNewPacket> {
public:
  AVNewPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,int32_t Size);
};

class AVPacketUnref : public WasmEdgeFFmpegAVCodec<AVPacketUnref> {
public:
  AVPacketUnref(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVGrowPacket : public WasmEdgeFFmpegAVCodec<AVGrowPacket> {
public:
  AVGrowPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,int32_t Size);
};

class AVShrinkPacket : public WasmEdgeFFmpegAVCodec<AVShrinkPacket> {
public:
  AVShrinkPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId, int32_t Size);
};

class AVReadFrame : public WasmEdgeFFmpegAVCodec<AVReadFrame> {
public:
  AVReadFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId, int32_t Size);
};

class AVPacketStreamIndex : public WasmEdgeFFmpegAVCodec<AVPacketStreamIndex> {
public:
  AVPacketStreamIndex(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge

