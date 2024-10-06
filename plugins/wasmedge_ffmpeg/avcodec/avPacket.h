// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVPacketAlloc : public HostFunction<AVPacketAlloc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPacketPtr);
};

class AVNewPacket : public HostFunction<AVNewPacket> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVPacketRef : public HostFunction<AVPacketRef> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t DestPacketId, uint32_t SrcPacketId);
};

class AVPacketUnref : public HostFunction<AVPacketUnref> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVGrowPacket : public HostFunction<AVGrowPacket> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVShrinkPacket : public HostFunction<AVShrinkPacket> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Size);
};

class AVPacketStreamIndex : public HostFunction<AVPacketStreamIndex> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetStreamIndex : public HostFunction<AVPacketSetStreamIndex> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t StreamIdx);
};

class AVPacketSize : public HostFunction<AVPacketSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketFlags : public HostFunction<AVPacketFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetFlags : public HostFunction<AVPacketSetFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int32_t Flags);
};

class AVPacketPos : public HostFunction<AVPacketPos> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetPos : public HostFunction<AVPacketSetPos> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Pos);
};

class AVPacketDuration : public HostFunction<AVPacketDuration> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetDuration : public HostFunction<AVPacketSetDuration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Duration);
};

class AVPacketDts : public HostFunction<AVPacketDts> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetDts : public HostFunction<AVPacketSetDts> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Dts);
};

class AVPacketPts : public HostFunction<AVPacketPts> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketSetPts : public HostFunction<AVPacketSetPts> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       int64_t Pts);
};

class AVPacketIsDataNull : public HostFunction<AVPacketIsDataNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId);
};

class AVPacketData : public HostFunction<AVPacketData> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvPacketId,
                       uint32_t DataPtr, uint32_t DataLen);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
