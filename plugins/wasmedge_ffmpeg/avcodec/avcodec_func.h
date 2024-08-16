// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecAllocContext3 : public HostFunction<AVCodecAllocContext3> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t AvCodecCtxPtr);
};

class AVCodecParametersFromContext
    : public HostFunction<AVCodecParametersFromContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId, uint32_t AvCodecCtxId);
};

class AVCodecParametersFree : public HostFunction<AVCodecParametersFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId);
};

class AVCodecFreeContext : public HostFunction<AVCodecFreeContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecParametersAlloc : public HostFunction<AVCodecParametersAlloc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamPtr);
};

class AVCodecGetType : public HostFunction<AVCodecGetType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecOpen2 : public HostFunction<AVCodecOpen2> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t AvCodecId,
                       uint32_t AvDictionaryId);
};

class AVCodecFindDecoder : public HostFunction<AVCodecFindDecoder> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ID,
                       uint32_t AvCodecId);
};

class AVCodecIsEncoder : public HostFunction<AVCodecIsEncoder> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecIsDecoder : public HostFunction<AVCodecIsDecoder> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecClose : public HostFunction<AVCodecClose> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecParametersToContext
    : public HostFunction<AVCodecParametersToContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t AvCodecParamId);
};

class AVCodecReceiveFrame : public HostFunction<AVCodecReceiveFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t FrameId);
};

class AVCodecSendPacket : public HostFunction<AVCodecSendPacket> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t PacketId);
};

class AVCodecFindEncoder : public HostFunction<AVCodecFindEncoder> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ID,
                       uint32_t AVCodecPtr);
};

class AVCodecReceivePacket : public HostFunction<AVCodecReceivePacket> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId, uint32_t PacketId);
};

class AVCodecSendFrame : public HostFunction<AVCodecSendFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId, uint32_t FrameId);
};

class AVCodecFindDecoderByName : public HostFunction<AVCodecFindDecoderByName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVCodecPtr,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVCodecFindEncoderByName : public HostFunction<AVCodecFindEncoderByName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVCodecPtr,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVPacketRescaleTs : public HostFunction<AVPacketRescaleTs> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVPacketId,
                       int32_t SrcNum, int32_t SrcDen, int32_t DestNum,
                       int32_t DestDen);
};

class AVPacketMakeWritable : public HostFunction<AVPacketMakeWritable> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVPacketId);
};

class AVCodecParametersCopy : public HostFunction<AVCodecParametersCopy> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVCodecParamId,
                       uint32_t StreamIdx);
};

class AVCodecVersion : public HostFunction<AVCodecVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecFlushBuffers : public HostFunction<AVCodecFlushBuffers> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId);
};

class AVCodecConfigurationLength
    : public HostFunction<AVCodecConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecConfiguration : public HostFunction<AVCodecConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVCodecLicenseLength : public HostFunction<AVCodecLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecLicense : public HostFunction<AVCodecLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
