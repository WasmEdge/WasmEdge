#pragma once
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
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPacketPtr);
};

class AVNewPacket : public WasmEdgeFFmpegAVCodec<AVNewPacket> {
public:
  AVNewPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVPacketRef : public WasmEdgeFFmpegAVCodec<AVPacketRef> {
public:
  AVPacketRef(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t DestPacketId, uint32_t SrcPacketId);
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
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVShrinkPacket : public WasmEdgeFFmpegAVCodec<AVShrinkPacket> {
public:
  AVShrinkPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVPacketStreamIndex : public WasmEdgeFFmpegAVCodec<AVPacketStreamIndex> {
public:
  AVPacketStreamIndex(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetStreamIndex
    : public WasmEdgeFFmpegAVCodec<AVPacketSetStreamIndex> {
public:
  AVPacketSetStreamIndex(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t StreamIdx);
};

class AVPacketSize : public WasmEdgeFFmpegAVCodec<AVPacketSize> {
public:
  AVPacketSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketFlags : public WasmEdgeFFmpegAVCodec<AVPacketFlags> {
public:
  AVPacketFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetFlags : public WasmEdgeFFmpegAVCodec<AVPacketSetFlags> {
public:
  AVPacketSetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Flags);
};

class AVPacketPos : public WasmEdgeFFmpegAVCodec<AVPacketPos> {
public:
  AVPacketPos(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetPos : public WasmEdgeFFmpegAVCodec<AVPacketSetPos> {
public:
  AVPacketSetPos(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Pos);
};

class AVPacketDuration : public WasmEdgeFFmpegAVCodec<AVPacketDuration> {
public:
  AVPacketDuration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetDuration : public WasmEdgeFFmpegAVCodec<AVPacketSetDuration> {
public:
  AVPacketSetDuration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Duration);
};

class AVPacketDts : public WasmEdgeFFmpegAVCodec<AVPacketDts> {
public:
  AVPacketDts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetDts : public WasmEdgeFFmpegAVCodec<AVPacketSetDts> {
public:
  AVPacketSetDts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Dts);
};

class AVPacketPts : public WasmEdgeFFmpegAVCodec<AVPacketPts> {
public:
  AVPacketPts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetPts : public WasmEdgeFFmpegAVCodec<AVPacketSetPts> {
public:
  AVPacketSetPts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Pts);
};

class AVPacketIsDataNull : public WasmEdgeFFmpegAVCodec<AVPacketIsDataNull> {
public:
  AVPacketIsDataNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketData : public WasmEdgeFFmpegAVCodec<AVPacketData> {
public:
  AVPacketData(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       uint32_t DataPtr, uint32_t DataLen);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
